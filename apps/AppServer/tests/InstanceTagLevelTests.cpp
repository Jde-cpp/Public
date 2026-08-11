#include <jde/db/db.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/QLAwait.h>
#include "helpers.h"
#include "../src/LogData.h"
#include "../src/ql/AppServerQL.h"
#define let const auto

//the per-instance log-level overrides: updateInstanceTagLevel mutations and the instanceTagLevels query, driven
//through the same AppServerQL custom hooks the web ui uses.
namespace Jde::App::Server::Tests{

	struct InstanceTagLevelTests : ::testing::Test{
		Ω RunQL( string query, jobject vars={} )->jvalue{
			return BlockAwait<QL::QLAwait<jvalue>,jvalue>( QL::QLAwait<jvalue>{move(query), move(vars), Jde::UserPK{Jde::UserPK::System}, Server::QLPtr()} );
		}
		Ω TagRows( ProgInstPK instanceId )->vector<DB::Row>{
			let table = Server::AppSchema()->GetView( "instance_tag_levels" ).DBName;
			return Server::AppSchema()->DS()->Select( {Ƒ("select type, tag, level_id from {} where instance_id=?", table), {DB::Value{instanceId}}} );
		}
		Ω MintInstance( str name )->ProgInstPK{
			let [program, instance, connection] = App::AddConnection( "Tests.TagLevels", name, "taglevel-host", 115 );
			return instance;
		}
	};

	TEST_F( InstanceTagLevelTests, UpsertAndQuery ){
		let instanceId = MintInstance( "upsert" );
		RunQL( Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"text\":{{\"default\":\"Warning\",\"sql\":\"Debug\"}}, \"binary\":{{\"default\":\"Debug\"}} )", instanceId) );
		let rows = TagRows( instanceId );
		ASSERT_EQ( rows.size(), 3u );
		auto foundSql = false;
		for( auto&& row : rows ){
			if( row.GetString(0)=="text" && row.GetUInt(1)==underlying(ELogTags::Sql) ){
				foundSql = true;
				EXPECT_EQ( row.GetUInt8Opt(2).value_or(0), (uint8)underlying(ELogLevel::Debug) );
			}
		}
		EXPECT_TRUE( foundSql );

		auto y = RunQL( Ƒ("instanceTagLevels( id: {} ){{ text binary }}", instanceId) );
		let& o = y.as_object();
		ASSERT_TRUE( o.contains("text") );
		ASSERT_TRUE( o.contains("binary") );
		let& text = o.at( "text" ).as_object();
		EXPECT_EQ( text.at("default").as_string(), "Warning" );
		EXPECT_EQ( text.size(), 2u );
		auto foundSqlKey = false;//the non-default tag key: currently serialized via ToString(tags, outputArray=true), so it may round-trip as `["sql"]` rather than `sql`.
		for( let& kv : text )
			foundSqlKey |= string{kv.key()}.contains( "sql" );
		EXPECT_TRUE( foundSqlKey );
		let& binary = o.at( "binary" ).as_object();
		EXPECT_EQ( binary.at("default").as_string(), "Debug" );
	}

	//only the requested groups come back.
	TEST_F( InstanceTagLevelTests, QueryFiltersColumns ){
		let instanceId = MintInstance( "filters" );
		RunQL( Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"text\":{{\"default\":\"Information\"}}, \"binary\":{{\"default\":\"Debug\"}} )", instanceId) );
		auto y = RunQL( Ƒ("instanceTagLevels( id: {} ){{ text }}", instanceId) );
		let& o = y.as_object();
		EXPECT_TRUE( o.contains("text") );
		EXPECT_FALSE( o.contains("binary") );
		EXPECT_FALSE( o.contains("appServer") );
	}

	//a null level deletes the override instead of upserting it.
	TEST_F( InstanceTagLevelTests, NullLevelDeletes ){
		let instanceId = MintInstance( "deletes" );
		RunQL( Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"text\":{{\"default\":\"Warning\",\"sql\":\"Debug\"}} )", instanceId) );
		ASSERT_EQ( TagRows(instanceId).size(), 2u );
		RunQL( Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"text\":{{\"sql\":null}} )", instanceId) );
		let rows = TagRows( instanceId );
		ASSERT_EQ( rows.size(), 1u );
		EXPECT_EQ( rows[0].GetUInt(1), 0u ) << "only the default override should remain";
	}

	//the appServer group rides the same table with its own type discriminator.
	TEST_F( InstanceTagLevelTests, AppServerGroup ){
		let instanceId = MintInstance( "appServer" );
		RunQL( Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"appServer\":{{\"default\":\"Trace\"}} )", instanceId) );
		let rows = TagRows( instanceId );
		ASSERT_EQ( rows.size(), 1u );
		EXPECT_EQ( rows[0].GetString(0), "appServer" );

		auto y = RunQL( Ƒ("instanceTagLevels( id: {} ){{ appServer }}", instanceId) );
		let& o = y.as_object();
		ASSERT_TRUE( o.contains("appServer") );
		EXPECT_EQ( o.at("appServer").as_object().at("default").as_string(), "Trace" );
	}
}
