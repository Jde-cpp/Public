//Everything Jde.App.Shared does to a log archive once it is in memory:  which entries a QL filter keeps, which strings
//come with them, the sort, the paging and the json the `logs` query returns.  ArchiveFile is fed the protos ArchiveAwait
//and ProtoLog would have read off disk, so no file and no data source is involved.
#include <gtest/gtest.h>
#include <boost/uuid/uuid_io.hpp>
#include <jde/fwk/chrono.h>
#include <jde/app/log/ArchiveFile.h>
#include "helpers.h"

#define let const auto

namespace Jde::App::Tests{
	Ω load( const vector<Logging::Entry>& entries, QL::TableQL& ql )ε->ArchiveFile{
		ArchiveFile y;
		y.Append( ql, archiveProto(entries) );
		return y;
	}
	Ω load( const vector<Logging::Entry>& entries )ε->ArchiveFile{
		auto ql = table( "logs" );
		return load( entries, ql );
	}
	Ω lines( const vector<Log::Proto::LogEntryFile>& entries )ι->vector<uint32_t>{
		vector<uint32_t> y;
		for( let& e : entries )
			y.push_back( e.line() );
		return y;
	}

	TEST( ArchiveFileTests, EntrySizeCountsEveryTimeBucket ){
		let af = load( {entry(tp(0), ELogLevel::Information, 1, "a"), entry(tp(0), ELogLevel::Information, 2, "b"), entry(tp(1), ELogLevel::Information, 3, "c")} );
		EXPECT_EQ( af.Entries.size(), 2u ); //keyed by time - two of the three share a timestamp.
		EXPECT_EQ( af.EntrySize(), 3u );
	}

	TEST( ArchiveFileTests, NoFilterKeepsEverything ){
		EXPECT_EQ( load({entry(tp(0), ELogLevel::Error, 1, "a"), entry(tp(1), ELogLevel::Debug, 2, "b")}).EntrySize(), 2u );
	}

	TEST( ArchiveFileTests, FilterSelectsEntries ){
		auto ql = table( "logs", "{level: 4}" ); //Error
		let af = load( {entry(tp(0), ELogLevel::Error, 1, "boom"), entry(tp(1), ELogLevel::Debug, 2, "noise")}, ql );
		ASSERT_EQ( af.EntrySize(), 1u );
		EXPECT_EQ( af.Entries.begin()->second.front().line(), 1u );
	}

	TEST( ArchiveFileTests, TagFilterIsBitwise ){
		auto ql = table( "logs", Ƒ("{{tags: {}}}", (uint)ELogTags::Test) );
		let af = load( {entry(tp(0), ELogLevel::Information, 1, "tagged", {}, "src/x.cpp", "Fn", ELogTags::Test), entry(tp(1), ELogLevel::Information, 2, "other", {}, "src/x.cpp", "Fn", ELogTags::Sql)}, ql );
		ASSERT_EQ( af.EntrySize(), 1u );
		EXPECT_EQ( af.Entries.begin()->second.front().line(), 1u );
	}

	//The archived-file path loads the whole string table before it filters, so a dropped entry's strings stay behind.
	TEST( ArchiveFileTests, ArchiveProtoKeepsEveryStringItWasGiven ){
		auto ql = table( "logs", "{level: 4}" );
		let af = load( {entry(tp(0), ELogLevel::Error, 1, "boom"), entry(tp(1), ELogLevel::Debug, 2, "noise")}, ql );
		ASSERT_EQ( af.EntrySize(), 1u );
		EXPECT_EQ( af.Templates.size(), 2u );
	}

	//The daily-file path partitions strings from entries first, then keeps only the ones a surviving entry names.
	TEST( ArchiveFileTests, FileEntriesKeepOnlyReferencedStrings ){
		let entries = vector<Logging::Entry>{ entry(tp(0), ELogLevel::Error, 1, "boom", {"42"}), entry(tp(1), ELogLevel::Debug, 2, "noise") };
		auto ql = table( "logs", "{level: 4}" );
		let filter = ql.Filter();
		ArchiveFile af{ filter, fileEntries(entries) };
		ASSERT_EQ( af.EntrySize(), 1u );
		ASSERT_EQ( af.Templates.size(), 1u );
		EXPECT_EQ( af.Templates.at(entries[0].Id()), "boom" );
		EXPECT_FALSE( af.Templates.contains(entries[1].Id()) );
		ASSERT_EQ( af.Args.size(), 1u );
		EXPECT_EQ( af.Args.at(Logging::Entry::GenerateId("42")), "42" );
	}

	//The app server writes gateway/opc entries with the originating app stamped on them;  the archive keeps the entry and
	//drops the attribution - LogProto::ToEntry has nowhere to put it.
	TEST( ArchiveFileTests, ExternalEntriesAreAppended ){
		let e = entry( tp(0), ELogLevel::Warning, 7, "external" );
		Log::Proto::ArchiveFile proto;
		*proto.add_externalentries() = LogProto::LogEntryFile( e, 3, 4 );
		addStrings( proto, e );

		ArchiveFile af;
		auto ql = table( "logs" );
		af.Append( ql, move(proto) );
		ASSERT_EQ( af.EntrySize(), 1u );
		EXPECT_EQ( af.Entries.begin()->second.front().line(), 7u );
	}

	//IsComplete is what stops the reader walking further back through the archived days.  It can only answer for a
	//time-ordered page:  any other order needs every entry before the first one can be named.
	TEST( ArchiveFileTests, IsComplete ){
		let af = load( {entry(tp(0), ELogLevel::Information, 1, "a"), entry(tp(1), ELogLevel::Information, 2, "b"), entry(tp(2), ELogLevel::Information, 3, "c")} );
		EXPECT_FALSE( af.IsComplete(table("logs", R"({orderBy: "time"})")) );              //no limit -> read it all.
		EXPECT_FALSE( af.IsComplete(table("logs", R"({limit: 2})")) );                     //no orderBy.
		EXPECT_FALSE( af.IsComplete(table("logs", R"({limit: 2, orderBy: "level"})")) );
		EXPECT_TRUE( af.IsComplete(table("logs", R"({limit: 2, orderBy: "time"})")) );
		EXPECT_TRUE( af.IsComplete(table("logs", R"({limit: 3, orderBy: "time"})")) );
		EXPECT_FALSE( af.IsComplete(table("logs", R"({limit: 3, offset: 1, orderBy: "time"})")) ); //offset counts against the limit.
	}

	TEST( ArchiveFileTests, SortDefaultsToTimeAscending ){
		let af = load( {entry(tp(2), ELogLevel::Information, 3, "c"), entry(tp(0), ELogLevel::Information, 1, "a"), entry(tp(1), ELogLevel::Information, 2, "b")} );
		EXPECT_EQ( lines(af.Sort({})), (vector<uint32_t>{1,2,3}) );
	}

	TEST( ArchiveFileTests, SortByLevelDescending ){
		let af = load( {entry(tp(0), ELogLevel::Debug, 1, "a"), entry(tp(1), ELogLevel::Critical, 2, "b"), entry(tp(2), ELogLevel::Information, 3, "c")} );
		auto ql = table( "logs", R"({orderBy: {level: "desc"}})" );
		EXPECT_EQ( lines(af.Sort(ql.OrderByJson())), (vector<uint32_t>{2,3,1}) );
	}

	//"message" is the formatted text, not the template:  sorting by it has to expand the args first.
	TEST( ArchiveFileTests, SortByMessageFormatsTheTemplate ){
		let af = load( {entry(tp(0), ELogLevel::Information, 1, "zeta"), entry(tp(1), ELogLevel::Information, 2, "alpha {}", {"9"})} );
		auto ql = table( "logs", R"({orderBy: "message"})" );
		EXPECT_EQ( lines(af.Sort(ql.OrderByJson())), (vector<uint32_t>{2,1}) ); //"alpha 9" < "zeta".
	}

	TEST( ArchiveFileTests, FilterOnMessageFormatsTheTemplate ){
		auto ql = table( "logs", R"({message: "alpha 9"})" );
		let af = load( {entry(tp(0), ELogLevel::Information, 1, "zeta"), entry(tp(1), ELogLevel::Information, 2, "alpha {}", {"9"})}, ql );
		ASSERT_EQ( af.EntrySize(), 1u );
		EXPECT_EQ( af.Entries.begin()->second.front().line(), 2u );
	}

	TEST( ArchiveFileTests, SubTableFiltersFileNameAndUser ){
		let entries = vector<Logging::Entry>{
			entry( tp(0), ELogLevel::Information, 1, "a", {}, "keep.cpp", "Fn", ELogTags::Test, UserPK{7} ),
			entry( tp(1), ELogLevel::Information, 2, "b", {}, "drop.cpp", "Fn", ELogTags::Test, UserPK{7} ),
			entry( tp(2), ELogLevel::Information, 3, "c", {}, "keep.cpp", "Fn", ELogTags::Test, UserPK{8} ) };

		auto byFile = table( "logs" );
		addTable( byFile, "file", {"name"}, R"({name: "keep.cpp"})" );
		EXPECT_EQ( lines(load(entries, byFile).Sort({})), (vector<uint32_t>{1,3}) );

		auto byUser = table( "logs" );
		addTable( byUser, "user", {"id"}, "{id: 8}" );
		EXPECT_EQ( lines(load(entries, byUser).Sort({})), (vector<uint32_t>{3}) );
	}

	TEST( ArchiveFileTests, ToJsonEntryColumns ){
		let e = entry( tp(0), ELogLevel::Error, 42, "boom {}", {"9"}, "src/x.cpp", "Fn", ELogTags::Test, UserPK{7} );
		let af = load( {e} );

		auto ql = table( "logs" );
		addTable( ql, "entries", {"templateId","argIds","level","tags","line","time","userId","fileId","functionId"} );
		let o = af.ToJson( ql );
		let& jentries = o.at( "entries" ).as_array();
		ASSERT_EQ( jentries.size(), 1u );
		let& j = jentries[0].as_object();
		EXPECT_EQ( j.at("templateId").as_string(), to_string(e.Id()) );
		EXPECT_EQ( j.at("fileId").as_string(), to_string(e.FileId()) );
		EXPECT_EQ( j.at("functionId").as_string(), to_string(e.FunctionId()) );
		EXPECT_EQ( j.at("level").as_string(), "Error" );
		EXPECT_EQ( j.at("line").to_number<uint32_t>(), 42u );
		EXPECT_EQ( j.at("userId").to_number<uint32_t>(), 7u );
		EXPECT_EQ( j.at("time").as_string(), ToIsoString(tp(0))+"Z" );
		let& tags = j.at( "tags" ).as_array();
		ASSERT_EQ( tags.size(), 1u );
		EXPECT_EQ( tags[0].as_string(), "test" );
		let& args = j.at( "argIds" ).as_array();
		ASSERT_EQ( args.size(), 1u );
		EXPECT_EQ( args[0].as_string(), to_string(Logging::Entry::GenerateId("9")) );
	}

	//Only the columns the query asked for are emitted - the web client trims on what it requested.
	TEST( ArchiveFileTests, ToJsonEmitsOnlyRequestedColumns ){
		let af = load( {entry(tp(0), ELogLevel::Error, 42, "boom")} );
		auto ql = table( "logs" );
		addTable( ql, "entries", {"line"} );
		let o = af.ToJson( ql );
		let& j = o.at( "entries" ).as_array()[0].as_object();
		EXPECT_EQ( j.size(), 1u );
		EXPECT_TRUE( j.contains("line") );
	}

	//No `entries` sub-table -> no entries key at all, rather than an empty array.
	TEST( ArchiveFileTests, ToJsonWithoutEntriesTable ){
		let af = load( {entry(tp(0), ELogLevel::Error, 42, "boom")} );
		auto ql = table( "logs" );
		EXPECT_FALSE( af.ToJson(ql).contains("entries") );
	}

	TEST( ArchiveFileTests, ToJsonPages ){
		let af = load( {entry(tp(0), ELogLevel::Information, 1, "a"), entry(tp(1), ELogLevel::Information, 2, "b"), entry(tp(2), ELogLevel::Information, 3, "c")} );
		auto ql = table( "logs", "{limit: 1, offset: 1}" );
		addTable( ql, "entries", {"line"} );
		let o = af.ToJson( ql );
		let& jentries = o.at( "entries" ).as_array();
		ASSERT_EQ( jentries.size(), 1u );
		EXPECT_EQ( jentries[0].as_object().at("line").to_number<uint32_t>(), 2u );
	}

	//`strings` is the id->text side table the client joins the ids against.  It is filled by the id columns that were
	//asked for, so a query with no id column gets an empty one.
	TEST( ArchiveFileTests, ToJsonStringsTable ){
		let e = entry( tp(0), ELogLevel::Error, 42, "boom {}", {"9"}, "src/x.cpp", "Fn" );
		let af = load( {e} );

		auto ql = table( "logs" );
		addTable( ql, "entries", {"templateId","argIds","fileId","functionId"} );
		addTable( ql, "strings", {"id","value"} );
		let o = af.ToJson( ql );
		flat_map<string,string> strings;
		for( let& s : o.at("strings").as_array() )
			strings.emplace( string{s.as_object().at("id").as_string()}, string{s.as_object().at("value").as_string()} );

		ASSERT_EQ( strings.size(), 4u );
		EXPECT_EQ( strings.at(to_string(e.Id())), "boom {}" );          //the template, not the formatted message.
		EXPECT_EQ( strings.at(to_string(e.FileId())), "src/x.cpp" );
		EXPECT_EQ( strings.at(to_string(e.FunctionId())), "Fn" );
		EXPECT_EQ( strings.at(to_string(Logging::Entry::GenerateId("9"))), "9" );
	}

	TEST( ArchiveFileTests, ToJsonSubTables ){
		let e = entry( tp(0), ELogLevel::Error, 42, "boom", {}, "src/x.cpp", "Fn", ELogTags::Test, UserPK{7} );
		let af = load( {e} );

		auto ql = table( "logs" );
		addTable( ql, "entries", {"line"} );
		addTable( ql, "file", {"id","name"} );
		addTable( ql, "function", {"name"} );
		addTable( ql, "user", {"id"} );
		let o = af.ToJson( ql );
		let& j = o.at( "entries" ).as_array()[0].as_object();
		EXPECT_EQ( j.at("file").as_object().at("name").as_string(), "src/x.cpp" );
		EXPECT_EQ( j.at("file").as_object().at("id").as_string(), to_string(e.FileId()) );
		EXPECT_EQ( j.at("function").as_object().at("name").as_string(), "Fn" );
		EXPECT_FALSE( j.at("function").as_object().contains("id") ); //not asked for.
		EXPECT_EQ( j.at("user").as_object().at("id").to_number<uint32_t>(), 7u );
	}
}
