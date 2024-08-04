#include "gtest/gtest.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"
#include "helpers.h"
#include <jde/iot/types/OpcServer.h>
#include <jde/io/Json.h>
#include <jde/iot/UM.h>
#include <jde/iot/IotGraphQL.h>

#define var const auto
namespace Jde::Iot{
	static sp<LogTag> _logTag{ Logging::Tag("tests") };

	class OpcServerTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void{};
		static uint OpcProviderId;
	};
	uint OpcServerTests::OpcProviderId{};

	TEST_F( OpcServerTests, InsertFailed ){
		var target = OpcServerTarget;
		json jInsert = Json::Parse(𐢜("{{\"input\":{{\"target\":\"{}\"}}}}", target) );
		DB::MutationQL insert{ "opcServer", DB::EMutationQL::Create, jInsert, nullopt };

		auto readJson = SelectOpcServer();
		var pk = readJson.contains( "id" ) ? readJson["id"].get<uint32>() : CreateOpcServer();

		DB::DataSourcePtr()->Execute(	𐢜("delete from iot_opc_servers where id='{}'", pk) ); //insert checks if failed because exists.
		VFuture( move(*GetHook()->InsertFailure(insert, 0)) ).get();
		ASSERT_EQ( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );
	}

	TEST_F( OpcServerTests, PurgeFailed ){
		var target = OpcServerTarget;
		var pk = CreateOpcServer();
		Future<ProviderPK>( ProviderAwait{target, false} ).get();//BeforPurge mock.
		json jPurge = Json::Parse( 𐢜("{{\"id\": {}}}", pk) );
		DB::MutationQL purge{ "opcServer", DB::EMutationQL::Purge, jPurge, nullopt };
		VFuture( move(*GetHook()->PurgeFailure(purge, 0)) ).get();
		ASSERT_LT( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );

		PurgeOpcServer( pk );
	}

	TEST_F( OpcServerTests, Crud ){
		var createdId = CreateOpcServer();

		var selectAll = "query opcServers{ id name attributes created updated deleted target description certificateUri isDefault url }";
		var selectAllJson = DB::Query( selectAll, 0 );
		TRACE( "selectAllJson={}", selectAllJson.dump() );
		var id = selectAllJson["data"]["opcServers"][0]["id"].get<OpcPK>();
		ASSERT_EQ( createdId, id );

		auto readJson = SelectOpcServer();
		ASSERT_EQ( readJson["id"].get<uint32>(), id );
		var target = Json::Getε<string>( readJson, "target" );
		ASSERT_NE( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );

		var description = "new description";
		var update = 𐢜( "{{ mutation {{ updateOpcServer( 'id':{}, 'input': {{'description':'{}'}} ) }} }}", id, description );
		var updateJson = DB::Query( Str::Replace(update, '\'', '"'), 0 );
		TRACE( "updateJson={}", updateJson.dump() );
		var updated = SelectOpcServer( id );
		ASSERT_EQ( description, Json::Getε(updated, "description") );

		var del = 𐢜( "{{mutation {{ deleteOpcServer('id':{}) }} }}", id );
		var deleteJson = DB::Query( Str::Replace(del, '\'', '"'), 0 );
		TRACE( "deleted={}", deleteJson.dump() );
	 	auto readJson2 = SelectOpcServer( id );
		ASSERT_FALSE( readJson2["deleted"].is_null() );

		PurgeOpcServer( id );
		ASSERT_EQ( Future<OpcServer>(OpcServer::Select(id, true)).get(), nullptr );
		ASSERT_EQ( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );
	}
}