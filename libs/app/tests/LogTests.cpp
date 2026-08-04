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
		std::this_thread::sleep_for( 1s );
		DBG( "archiveFile: {}", archiveFile.string() );
		ASSERT_TRUE( fs::exists(archiveFile) );
		auto content = BlockTAwait<string>( IO::ReadAwait(archiveFile) );
		ASSERT_NO_THROW( Protobuf::Deserialize<App::Log::Proto::ArchiveFile>(move(content)) );
	}

	TEST_F( LogTests, ArchiveExternal ){
		let archiveFile = ArchiveFile( 2 );
		if( fs::exists(archiveFile) )
			fs::remove( archiveFile );
		let id = Round( 2, "External test message", 100, std::pair<uint32,uint32>{123,456} );
		std::this_thread::sleep_for( 1s );
		ASSERT_TRUE( fs::exists(archiveFile) );
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
}
