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
		Ω SetUpTestCase()ι->void{
			auto p = mu<IotGraphQL>();
			_pHook = p.get();
			DB::GraphQL::Hook::Add( move(p) );
		};
		static IotGraphQL* _pHook;
		static uint OpcProviderId;
	};
	uint OpcServerTests::OpcProviderId{};
	IotGraphQL* OpcServerTests::_pHook{};

	TEST_F( OpcServerTests, CreateInsertFailed ){
		var target = OpcServerTarget;
		var pk = CreateOpcServer();
		DB::MutationQL purge{ "opcServer", DB::EMutationQL::Create, { {"id", pk} }, nullopt };
		DB::MutationQL insert{ "opcServer", DB::EMutationQL::Create, { {"input", {"target", target}} }, nullopt };
		VFuture( move(*_pHook->PurgeFailure(purge)) );
		ASSERT_EQ( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );
		VFuture( move(*_pHook->InsertFailure(insert)) );
		ASSERT_EQ( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );
		PurgeOpcServer( pk );
	}

	TEST_F( OpcServerTests, Crud ){
		var createdId = CreateOpcServer();
		
		var selectAll = "query{ opcServers { id name attributes created updated deleted target description certificateUri isDefault url } }";
		var selectAllJson = DB::Query( selectAll, 0 );
		TRACE( "selectAllJson={}", selectAllJson.dump() );
		var id = selectAllJson["data"]["opcServers"][0]["id"].get<OpcPK>();
		ASSERT_EQ( createdId, id );

		auto readJson = SelectOpcServer();
		ASSERT_EQ( readJson["id"].get<uint32>(), id );
		var target = Json::Getε<string>( readJson, "target" );
		ASSERT_NE( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );

		var description = "new description";
		var update = Jde::format( "{{ mutation {{ updateOpcServer( 'id':{}, 'input': {{'description':'{}'}} ) }} }}", id, description );
		var updateJson = DB::Query( Str::Replace(update, '\'', '"'), 0 );
		TRACE( "updateJson={}", updateJson.dump() );
		var updated = SelectOpcServer( id );
		ASSERT_EQ( description, Json::Getε(updated, "description") );

		var del = Jde::format( "{{mutation {{ deleteOpcServer('id':{}) }} }}", id );
		var deleteJson = DB::Query( Str::Replace(del, '\'', '"'), 0 );
		TRACE( "deleted={}", deleteJson.dump() );
	 	auto readJson2 = SelectOpcServer( id );
		ASSERT_FALSE( readJson2["deleted"].is_null() );

		PurgeOpcServer( id );
		ASSERT_EQ( Future<OpcServer>(OpcServer::Select(id, true)).get(), nullptr );
		ASSERT_EQ( 0, *Future<ProviderPK>(ProviderAwait{target}).get() );
	}
}