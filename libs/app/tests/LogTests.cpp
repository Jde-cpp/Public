//Ported from apps/OpcGateway/tests/LogTests.cpp - everything in that file that needs only Jde.App.Shared and a
//filesystem:  ProtoLog's daily file, the archive rounds it triggers, and reading both back through the QL await.
//Remote/Subscribe/LogTagsIntrospection stayed behind; they need a live app client and socket.
//Unlike the rest of this suite these are integration tests - ProtoLog flushes on a worker and the archive round is
//asynchronous - so they poll rather than assert immediately.
#include <gtest/gtest.h>
#include <thread>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/FileAwait.h>
#include <jde/fwk/io/protobuf.h>
#include <jde/fwk/str.h>
#include <jde/ql/ql.h>
#include <jde/app/log/DailyLoadAwait.h>
#include <jde/app/log/LogQLAwait.h>
#include <jde/app/log/ProtoLog.h>

#define let const auto

namespace Jde::App::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	struct LogTests : ::testing::Test{
	protected:
		α Log()ι->App::ProtoLog&{ return Logging::GetLogger<App::ProtoLog>(); }
		//<root>/<year>/<month>/<day>/archive.binpb - one file per local day, so each test owns a date of its own.
		α ArchiveFile( uint day )ι->fs::path{ return Log().Root()/"2025"/"1"/std::to_string(day)/"archive.binpb"; }
		//An entry dated other than today flags ProtoLog for archiving; today-dated filler flips the day back and fills
		//the flush buffer, so the round runs without waiting out the delay.
		//Poll, never sleep a fixed interval.  A round rewrites *today's* archive whole, and that file grows with every run
		//the suite makes into the same tree - so a 1s sleep that passed on a clean checkout fails once the tree has some
		//history behind it.  Diagnosed after ~100 runs in one day left 20MB there and turned this from flaky to certain.
		α WaitForFile( const fs::path& path, Duration limit=20s )ι->bool{
			for( let deadline = Clock::now()+limit; Clock::now()<deadline; std::this_thread::sleep_for(25ms) ){
				if( fs::exists(path) )
					return true;
			}
			return fs::exists( path );
		}
		α Round( uint day, sv text, uint filler, optional<std::pair<uint32,uint32>> external={} )ι->uuid{
			Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, string{text} };
			e.Time = Chrono::ToTimePoint( 2025, 1, (uint8)day, 12 );
			if( external )
				Log().Write( e, external->first, external->second );
			else
				Log().Write( e );
			for( uint i=0; i<filler; ++i ){
				Log().Write( {SRCE_CUR, ELogLevel::Information, ELogTags::Test, Ƒ("{} filler {}", text, i)} );
				if( filler<=100 && fs::exists(ArchiveFile(day)) )
					break;
			}
			return e.Id();
		}
		α SetUp()->void override{ ASSERT_NO_THROW( Log() ); } //config/App.Tests.jsonnet configures /logging/proto.
	};

	TEST_F( LogTests, Exists ){
		Log().Write( {SRCE_CUR, ELogLevel::Information, ELogTags::Test, "DailyLoadAwait test message"} );
		auto entries = BlockAwait<TAwait<vector<App::Log::Proto::FileEntry>>,vector<App::Log::Proto::FileEntry>>( App::DailyLoadAwait(Log().DailyFile()) );
		ASSERT_TRUE( entries.size() ); //Query mode merges the unflushed buffer with the file, so this holds before any flush.
	}

	TEST_F( LogTests, Archive ){
		let archiveFile = ArchiveFile( 1 );
		if( fs::exists(archiveFile) )
			fs::remove( archiveFile );
		Round( 1, "Test message", 100 );
		DBG( "archiveFile: {}", archiveFile.string() );
		ASSERT_TRUE( WaitForFile(archiveFile) );
		auto content = BlockTAwait<string>( IO::ReadAwait(archiveFile) );
		ASSERT_NO_THROW( Protobuf::Deserialize<App::Log::Proto::ArchiveFile>(move(content)) );
	}

	TEST_F( LogTests, ArchiveExternal ){
		let archiveFile = ArchiveFile( 2 );
		if( fs::exists(archiveFile) )
			fs::remove( archiveFile );
		let id = Round( 2, "External test message", 100, std::pair<uint32,uint32>{123,456} );
		ASSERT_TRUE( WaitForFile(archiveFile) );
		auto content = BlockTAwait<string>( IO::ReadAwait(archiveFile) );
		App::Log::Proto::ArchiveFile archive;
		ASSERT_NO_THROW( archive = Protobuf::Deserialize<App::Log::Proto::ArchiveFile>(move(content)) );
		optional<App::Log::Proto::LogEntryFileExternal> external;
		for( int i=0; i<archive.externalentries_size() && !external; ++i ){
			if( let& x = archive.externalentries(i); x.app_pk()==123 && x.app_instance_pk()==456 )
				external = x;
		}
		ASSERT_TRUE( external );
		ASSERT_EQ( Protobuf::ToGuid(external->template_id()), id );
		bool templateArchived{};
		for( int i=0; i<archive.templates_size() && !templateArchived; ++i )
			templateArchived = archive.templates(i).value()=="External test message";
		ASSERT_TRUE( templateArchived ) << "External entry's template string not archived.";
	}

	// Regression, two ways a round could write an entry it had already archived:
	//   1. ArchiveFileAwait::Save appended the fully-merged archive to the very file it had just merged from, so every
	//      round wrote back everything already on disk - archive.binpb grew ~x3.7 per round until it no longer parsed
	//      and aborted the suite.
	//   2. the round archived ProtoLog's unflushed buffer as well as the daily file, but deleted only the file, so an
	//      entry still buffered when a round ran was archived again as soon as the next flush wrote it to disk.  Timing
	//      dependent - it needed a flush already in flight when the entry was written, so it only flaked (release more
	//      often than debug).
	// Either way, a second round must not duplicate the first round's entries.
	TEST_F( LogTests, ArchiveReplacesFile ){
		let archiveFile = ArchiveFile( 3 );
		if( fs::exists(archiveFile) )
			fs::remove( archiveFile );
		auto count = []( const App::Log::Proto::ArchiveFile& archive, const uuid& id )ι->uint{
			uint y{};
			for( int i=0; i<archive.entries_size(); ++i )
				y += Protobuf::ToGuid( archive.entries(i).template_id() )==id;
			return y;
		};
		//the archive is written asynchronously & rewritten in place, so a read can catch it mid-write - retry until
		//`until` lands, then let the round quiesce: the round's remaining entries are still arriving (and a later flush
		//can start another round), so a count sampled the instant an entry appears is still moving.
		auto archived = [&archiveFile,&count]( const uuid& until )ι->App::Log::Proto::ArchiveFile{
			App::Log::Proto::ArchiveFile y;
			for( int i=0; i<100; ++i ){
				try{
					if( fs::exists(archiveFile) ){
						auto archive = Protobuf::Deserialize<App::Log::Proto::ArchiveFile>( BlockTAwait<string>(IO::ReadAwait(archiveFile)) );
						if( count(archive, until) ){
							if( y.entries_size()==archive.entries_size() )//unchanged over the last interval - settled.
								return archive;
							y = move( archive );
						}
					}
				}
				catch( const Exception& )
				{}
				std::this_thread::sleep_for( 250ms );
			}
			return y;
		};
		let first = Round( 3, "ArchiveReplacesFile first", 200 );
		ASSERT_EQ( count(archived(first), first), 1u ) << "first round's entry not archived exactly once";
		let second = Round( 3, "ArchiveReplacesFile second", 200 );
		let after = archived( second );
		ASSERT_EQ( count(after, second), 1u ) << "second round's entry not archived exactly once";
		//exactly once, not "same as before": a round archives the daily file and nothing else, and holds that file's lock from the read until fs::remove - so an entry has one source, and no round can re-read what an earlier one archived.
		EXPECT_EQ( count(after, first), 1u ) << "the second round wrote the first round's entry again - archive.binpb was appended to, not replaced";
	}

	//#4: the string-dedup cache used to outlive the daily file a round deletes.  AddString suppresses a kStr whose id is
	//already cached, and a round builds its String table only from the kStr records still in the daily file - so a string
	//first emitted before a round was silently absent from every later archive, ArchiveFile::find returned Str::Empty(),
	//and the log page rendered a blank message/file/function for every historical entry.  Two rounds a day apart sharing
	//one template text is the whole reproduction: the second round's id is cached, so its String never gets written.
	TEST_F( LogTests, ArchiveStringsSurviveRound ){
		constexpr sv text{ "ArchiveStringsSurviveRound shared template" };
		for( let d : {10u, 11u} )
			if( fs::exists(ArchiveFile(d)) )
				fs::remove( ArchiveFile(d) );
		//an entry and the strings it references are written in one ArchiveFile message, so an archive that parses and holds
		//the entry holds whatever strings that write carried - no settle loop needed, only a retry past a partial read.
		auto archivedWith = []( const fs::path& file, const uuid& id )ι->optional<App::Log::Proto::ArchiveFile>{
			for( uint i=0; i<100; ++i ){
				try{
					if( fs::exists(file) ){
						auto archive = Protobuf::Deserialize<App::Log::Proto::ArchiveFile>( BlockTAwait<string>(IO::ReadAwait(file)) );
						for( int e=0; e<archive.entries_size(); ++e ){
							if( Protobuf::ToGuid(archive.entries(e).template_id())==id )
								return archive;
						}
					}
				}
				catch( const Exception& )
				{}
				std::this_thread::sleep_for( 250ms );
			}
			return nullopt;
		};
		auto templateValue = []( const App::Log::Proto::ArchiveFile& archive, const uuid& id )ι->optional<string>{
			for( int i=0; i<archive.templates_size(); ++i ){
				if( Protobuf::ToGuid(archive.templates(i).id())==id )
					return archive.templates( i ).value();
			}
			return nullopt;
		};

		let id = Round( 10, text, 200 );//both rounds share the text, so both share this md5 template id.
		let first = archivedWith( ArchiveFile(10), id );
		ASSERT_TRUE( first ) << "first round's entry never archived";
		ASSERT_EQ( templateValue(*first, id), string{text} );//sanity: the first round was never the broken one.

		ASSERT_EQ( Round(11, text, 200), id );
		let second = archivedWith( ArchiveFile(11), id );
		ASSERT_TRUE( second ) << "second round's entry never archived";
		EXPECT_EQ( templateValue(*second, id), string{text} ) << "the second round archived the entry with no String for its template - the dedup cache outlived the daily file the first round deleted";
	}

	//#3: _today used to be seeded from Clock::now(), so a service that started and stopped within one day never saw a day
	//change - _needsArchive was never set, nothing archived, and log.binpb accumulated across every restart cycle forever.
	//It is now seeded from the daily file, so a restart on a later day arms the round the previous run never ran.  These
	//construct their own ProtoLog over a scratch root rather than using the registered logger: the seeding happens exactly
	//once, in the constructor, so a restart is the only way to observe it.
	struct DailyFileDayTests : ::testing::Test{
	protected:
		fs::path _root;
		α SetUp()->void override{
			_root = Logging::GetLogger<App::ProtoLog>().Root()/"daily-file-day";
			fs::remove_all( _root );
			fs::create_directories( _root );
		}
		α TearDown()->void override{ fs::remove_all( _root ); }
		//delay PT24H so the scratch logger never flushes or arms a timer of its own - these assert the seed, not a round.
		α Make()ε->up<App::ProtoLog>{ return mu<App::ProtoLog>( jobject{{"path", _root.string()}, {"delay", "PT24H"}, {"timeZone", "America/New_York"}} ); }
		//content is irrelevant to the rule under test - the seed comes from the file's last-write time, never from a parse.
		α WriteDailyFile( TimePoint written )->void{
			let file = _root/"log.binpb";
			{ std::ofstream os{ file, std::ios::binary }; os << "unparsed"; }
			fs::last_write_time( file, fs::file_time_type::clock::from_sys(std::chrono::time_point_cast<fs::file_time_type::duration>(written)) );
		}
	};

	TEST_F( DailyFileDayTests, NoDailyFileSeedsToday ){
		let log = Make();
		EXPECT_EQ( log->Today(), Chrono::LocalYMD(Clock::now(), log->TimeZone()) );
	}

	TEST_F( DailyFileDayTests, StaleDailyFileSeedsItsOwnDay ){
		let written = Clock::now()-std::chrono::days{3};
		WriteDailyFile( written );
		let log = Make();
		let staleDay = Chrono::LocalYMD( written, log->TimeZone() );
		EXPECT_EQ( log->Today(), staleDay );//the restart that used to lose the round: seeded to today, every entry matched, nothing ever archived.
		EXPECT_NE( log->Today(), Chrono::LocalYMD(Clock::now(), log->TimeZone()) ) << "3 days is not a day change";
	}

	//a restart on the same day must not arm a round - the file is current and re-archiving it would split today's entries
	//across the daily file and today's archive for no reason.
	TEST_F( DailyFileDayTests, CurrentDailyFileSeedsToday ){
		WriteDailyFile( Clock::now() );
		let log = Make();
		EXPECT_EQ( log->Today(), Chrono::LocalYMD(Clock::now(), log->TimeZone()) );
	}

	//#7: _dailyFileStart is what LogAwait::ShouldReadLocal (`!_endTime || *_endTime > _dailyFileStart`) consults to decide
	//whether today's log can hold anything in range.  It has to be a *lower* bound on everything reachable locally - the
	//daily file and the unflushed buffer both - because reading a file with nothing in range costs one read, while skipping
	//one that does costs the entries.  The seeding cases need no writes, so they reuse the scratch-logger harness.
	struct DailyFileStartTests : DailyFileDayTests{};

	TEST_F( DailyFileStartTests, NoDailyFileStartsAtMax ){
		EXPECT_EQ( Make()->DailyFileStart(), TimePoint::max() );//nothing local at all - the one case where skipping is right.
	}

	TEST_F( DailyFileStartTests, ExistingDailyFileClaimsTheWidestBound ){
		WriteDailyFile( Clock::now() );
		EXPECT_EQ( Make()->DailyFileStart(), TimePoint::min() ) << "a file left by a previous run holds entries of unknown age - a bounded query has to read it";
	}

	//The bug proper.  A flush moves entries out of the buffer and into the daily file; both are local, so the bound must
	//survive it.  Parking it at max() made `*_endTime > max` false for every finite bound, so a query with any time filter
	//skipped the whole of today's log and answered out of the archives alone.
	TEST_F( LogTests, DailyFileStartSurvivesFlush ){
		//Retried, because one thing legitimately does release the bound: an archive round.  The tests above leave 2025-dated
		//entries behind, so the first flush here can find _needsArchive still armed, drain, and correctly reset to max() -
		//after which the filler still to come lowers it past `when`.  That is the fix working, not failing.  A retry is
		//sound rather than a way of hiding flakiness: the bug it guards is deterministic per flush, so before the fix every
		//attempt fails, and the round can only intervene once because the drain clears the flag.
		bool held{};
		for( uint attempt=0; attempt<5 && !held; ++attempt ){
			let when = Clock::now();
			let before = fs::exists( Log().DailyFile() ) ? fs::file_size( Log().DailyFile() ) : 0;
			//by size, not by waiting out /logging/proto/delay (PT1M in App.Tests.jsonnet); _delaySize is 8096 bytes.  All
			//dated now, so nothing here arms a round of its own.
			for( uint i=0; i<400; ++i )
				Log().Write( {SRCE_CUR, ELogLevel::Information, ELogTags::Test, Ƒ("DailyFileStartSurvivesFlush {} filler {}", attempt, i)} );
			bool flushed{};
			for( uint i=0; i<100 && !flushed; ++i ){
				flushed = fs::exists( Log().DailyFile() ) && fs::file_size( Log().DailyFile() )>before;
				if( !flushed )
					std::this_thread::sleep_for( 50ms );
			}
			ASSERT_TRUE( flushed ) << "no flush landed - the filler did not exceed _delaySize";
			held = Log().DailyFileStart()!=TimePoint::max() && Log().DailyFileStart()<=when;
		}
		EXPECT_TRUE( held ) << "a flush released the bound - it moves entries from the buffer into the daily file, and both are local, so every time-bounded logs() query would now skip the daily file";
	}

	//#9: the attribution check read ProgramPK/InstancePK, which nothing ever assigned - indeterminate, so any forwarded
	//entry whose ids happened to match the garbage was silently downgraded from kExternalEntry to kEntry and lost its
	//app_pk/app_instance_pk, differently on every process start.  They hold this process's own pks now.
	TEST_F( LogTests, ForwardedEntryAttribution ){
		constexpr App::ProgramPK app{ 4242 };
		constexpr App::ProgInstPK inst{ 8484 };
		//unique per run: the read below covers the daily file, which outlives the suite, and an earlier run's entry for the
		//same text would answer for this one - including a run from before the fix, whose answers were the wrong ones.
		let run = ToIsoString( Clock::now() );
		using enum Log::Proto::FileEntry::ValueCase;
		//via DailyLoadAwait, not Log().Entries(): the buffer alone loses the entry whenever the write that added it crossed
		//_delaySize and flushed, which is a coin toss depending on how full the earlier tests left the buffer.  Query mode
		//merges the buffer with the file, so the entry is found either way.
		auto write = [this,&run]( App::ProgramPK a, App::ProgInstPK i, sv what )ε->optional<Log::Proto::FileEntry::ValueCase>{
			Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, Ƒ("ForwardedEntryAttribution {} {}", what, run) };
			Log().Write( e, a, i );
			for( let& fe : BlockAwait<TAwait<vector<Log::Proto::FileEntry>>,vector<Log::Proto::FileEntry>>(App::DailyLoadAwait(Log().DailyFile())) ){
				if( fe.value_case()==kEntry && Protobuf::ToGuid(fe.entry().template_id())==e.Id() )
					return kEntry;
				if( fe.value_case()==kExternalEntry && Protobuf::ToGuid(fe.external_entry().template_id())==e.Id() )
					return kExternalEntry;
			}
			return nullopt;
		};
		EXPECT_EQ( write(app, inst, "unset"), kExternalEntry ) << "with our own pks unset nothing forwarded is ours";
		Log().SetAppPKs( app, inst );
		EXPECT_EQ( write(app, inst, "self"), kEntry ) << "both pks match - one of our own entries coming back";
		EXPECT_EQ( write(app, inst+1, "sibling"), kExternalEntry ) << "another instance of the same program is not us - matching the program pk alone must not drop attribution";
		EXPECT_EQ( write(0, inst, "unattributed"), kEntry ) << "no app pk to preserve";
		Log().SetAppPKs( 0, 0 );//the fixture shares the process-wide logger.
	}

	//#13(c): the archive-read lambda in ArchiveLoadAwait::LoadArchives was marked ι but calls Protobuf::Deserialize and
	//ArchiveFile::Append, both ε.  One corrupt or truncated archive.binpb - a torn write, a file left half-written by a
	//killed process - took the whole process down with std::terminate instead of failing the query that read it.
	TEST_F( LogTests, CorruptArchiveFailsTheQueryNotTheProcess ){
		let dir = Log().Root()/"2025"/"1"/"20";//a day of its own; ArchiveFiles() skips directories that are not a valid date.
		fs::create_directories( dir );
		{
			std::ofstream os{ dir/"archive.binpb", std::ios::binary };
			os << "not a protobuf";
		}
		//removed however this exits - every other test in the suite reads the archives too, and the tree outlives the run.
		struct Cleanup final{ fs::path Dir; ~Cleanup(){ std::error_code ec; fs::remove_all( Dir, ec ); } } cleanup{ dir };
		//no limit, so IsComplete() is false and ArchiveLoadAwait walks every archive file, this one included.
		EXPECT_ANY_THROW( BlockTAwait<jvalue>(App::LogQLAwait{move(QL::Parse(string{"logs{ entries{templateId time} }"}, {}, {}).Queries()[0])}) );
	}

	TEST_F( LogTests, GraphQL ){
		let now = ToIsoString( Clock::now() );
		Logging::Entry eNow{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, string{now} };
		Logging::Entry eHour{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, ToIsoString(eNow.Time - 1h) };
		eHour.Time = eNow.Time - 1h;
		TRACE( "prev.Time: {}, id: {}", ToIsoString(eHour.Time), Jde::ToString(eHour.Id()) );
		Log().Write( eHour );
		const string start{ ToIsoString(eHour.Time+1s) };
		TRACE( "filter: time: {}, id: {}", start, Jde::ToString(eNow.Id()) );
		Log().Write( eNow );
		constexpr auto q = "logs( time: {gt: $start} ){ entries{templateId argIds level tags line time userId fileId functionId} strings{id value} }";
		jobject vars{ {"start", start} };
		let logs = BlockTAwait<jvalue>( App::LogQLAwait{move(QL::Parse(q, vars, {}).Queries()[0])} );
		optional<jobject> jNow;
		for( let& log : logs.at("entries").as_array() ){
			let id = ToUuid( log.at("templateId").as_string() );
			if( id==eNow.Id() )
				jNow = log.as_object();
			ASSERT_FALSE( id==eHour.Id() ) << "Found hour log entry which should be excluded.";
		}
		ASSERT_TRUE( jNow );
	}

	//#5: the daily-file overload of ArchiveFile::Append filtered each entry before populating the string maps, but Test()
	//resolves "text"/"message"/"args" through those very maps - so every entry was compared against "" and any such filter
	//returned nothing from today's log.  Nothing surviving meant nothing was ever added, so it could not self-correct.
	//The time filter in GraphQL above never caught it: time is read off the entry, not out of a map.
	TEST_F( LogTests, DailyFileStringFilter ){
		//unique per run: the daily file and the archives outlive the suite, so a fixed marker would match older entries too.
		let marker = Ƒ( "DailyFileStringFilter {}", ToIsoString(Clock::now()) );
		Logging::Entry e{ SRCE_CUR, ELogLevel::Information, ELogTags::Test, string{marker} };
		Log().Write( e );
		//no time filter, so LogAwait reads the daily file (buffer included) - this is the ArchiveFile(Filter&,...) path.
		constexpr auto q = "logs( text: {eq: $text} ){ entries{templateId time} }";
		jobject vars{ {"text", marker} };
		let logs = BlockTAwait<jvalue>( App::LogQLAwait{move(QL::Parse(q, vars, {}).Queries()[0])} );
		uint matched{};
		for( let& log : logs.at("entries").as_array() )
			matched += ToUuid( log.at("templateId").as_string() )==e.Id();
		EXPECT_EQ( matched, 1u ) << "a text filter found nothing in the daily file - Append tested entries against string maps it had not filled yet";
	}
}
