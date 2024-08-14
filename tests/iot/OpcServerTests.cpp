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
	constexpr ELogTags _tags{ ELogTags::Test };

	struct OpcServerTests : ::testing::Test{
		atomic_flag Wait;
		std::any Result;
		std::any Id;
	protected:
		Ω SetUpTestCase()ι->void{};
		α SetUp()ι->void{ Wait.clear(); }
		static uint OpcProviderId;
		α InsertFailedImpl()ε->Task;
		α PurgeFailedImpl()ε->CreateOpcServerAwait::Task;
		α CrudImpl()ε->CreateOpcServerAwait::Task;
		α CrudImpl2( OpcPK id )ε->Task;
		α CrudPurge( OpcPK id )ε->PurgeOpcServerAwait::Task;
	};
	uint OpcServerTests::OpcProviderId{};

	α OpcServerTests::InsertFailedImpl()ε->Task{
		var target = OpcServerTarget;
		json jInsert = Json::Parse( 𐢜("{{\"input\":{{\"target\":\"{}\"}}}}", target) );
		DB::MutationQL insert{ "opcServer", DB::EMutationQL::Create, jInsert, nullopt };

		var existingProviderPK = *( co_await ProviderAwait{target} ).UP<ProviderPK>();
		var existingServer = SelectOpcServer();
		var existingOpcPK = Json::Get<OpcPK>( existingServer, "id" );
		if( !existingOpcPK && !existingProviderPK ){
			[insert,this]()->CreateOpcServerAwait::Task {
				auto pk = co_await CreateOpcServerAwait();
				DB::DataSourcePtr()->Execute(	𐢜("delete from iot_opc_servers where id='{}'", pk) ); //InsertFailed checks if failure occurs because exists.
				[=,this]()->Task{
					co_await *GetHook()->InsertFailure( insert, 0 );
					Wait.test_and_set();
					Wait.notify_one();
				}();
			}();
		}
		else{
			if( existingOpcPK )
				DB::DataSourcePtr()->Execute(	𐢜("delete from iot_opc_servers where id='{}'", existingOpcPK) ); //InsertFailed checks if failure occurs because exists.
			co_await *GetHook()->InsertFailure( insert, 0 );
			Wait.test_and_set();
			Wait.notify_one();
		}
	}

	TEST_F( OpcServerTests, InsertFailed ){
		InsertFailedImpl();
		Wait.wait( false );
		ASSERT_EQ( 0, *Future<ProviderPK>(ProviderAwait{OpcServerTarget}).get() );
	}

	α OpcServerTests::PurgeFailedImpl()ε->CreateOpcServerAwait::Task{
		var existingServer = SelectOpcServer();
		auto opcPK = Json::Get<OpcPK>( existingServer, "id" );
		if( !opcPK )
			opcPK = co_await CreateOpcServerAwait();
		Id = opcPK;
		[=,this]()->Task {
			var id = opcPK;
			auto self = this;
			co_await ProviderAwait{OpcServerTarget, false};//BeforePurge mock.
			json jPurge = json{ {"id", id} };
			DB::MutationQL purge{ "opcServer", DB::EMutationQL::Purge, jPurge, nullopt };
			co_await *GetHook()->PurgeFailure( purge, 0 );
			self->Result = *( co_await ProviderAwait{OpcServerTarget} ).UP<ProviderPK>();
			self->Wait.test_and_set();
			self->Wait.notify_one();
		}();
	}
	TEST_F( OpcServerTests, PurgeFailed ){
		PurgeFailedImpl();
		Wait.wait( false );
		ASSERT_NE( 0, std::any_cast<ProviderPK>(Result) );
		PurgeOpcServer( std::any_cast<OpcPK>(Id) );
	}

	α OpcServerTests::CrudImpl()ε->CreateOpcServerAwait::Task{
		var existingServer = SelectOpcServer();
		var existingOpcPK = Json::Get<OpcPK>( existingServer, "id" );
		if( existingOpcPK )
			PurgeOpcServer( existingOpcPK );
		var createdId = co_await CreateOpcServerAwait();
		var selectAll = "{ query opcServers{ id name attributes created updated deleted target description certificateUri isDefault url } }";
		var selectAllJson = DB::Query( selectAll, 0 );
		Trace( _tags, "selectAllJson={}", selectAllJson.dump() );
		var id = selectAllJson["data"]["opcServers"][0]["id"].get<OpcPK>();
		THROW_IF( createdId!=id, "createdId={} id={}", createdId, id );
		CrudImpl2( id );
	}

	α OpcServerTests::CrudImpl2( OpcPK id )ε->Task{
		auto readJson = SelectOpcServer();
		THROW_IF( Json::Getε<OpcPK>(readJson, "id")!=id, "id={} readJson={}", id, readJson.dump() );
		var target = Json::Getε( readJson, "target" );

		var providerId = *( co_await ProviderAwait{target} ).UP<ProviderPK>();
		THROW_IF( providerId==0, "providerId==0" );;

		var description = "new description";
		var update = 𐢜( "{{ mutation updateOpcServer( 'id':{}, 'input': {{'description':'{}'}} ) }}", id, description );
		var updateJson = DB::Query( Str::Replace(update, '\'', '"'), 0 );
		Trace( _tags, "updateJson={}", updateJson.dump() );
		var updated = SelectOpcServer( id );
		THROW_IF( Json::Getε( updated, "description" )!=description, "description={} updated={}", description, updated.dump() );

		var del = 𐢜( "{{mutation deleteOpcServer('id':{}) }}", id );
		var deleteJson = DB::Query( Str::Replace(del, '\'', '"'), 0 );
		Trace( _tags, "deleted={}", deleteJson.dump() );
	 	auto readJson2 = SelectOpcServer( id );
		THROW_IF( readJson2["deleted"].is_null(), "deleted failed" );

		CrudPurge( id );
	}

	α OpcServerTests::CrudPurge( OpcPK id )ε->PurgeOpcServerAwait::Task{
		co_await PurgeOpcServerAwait{ id };
		[]( OpcPK id, OpcServerTests& self )->Task {
			var opcServer = ( co_await OpcServer::Select(id, true) ).UP<OpcServer>();
			THROW_IF( opcServer, "Purge Failed" );
			self.Result = *( co_await ProviderAwait{OpcServerTarget} ).UP<ProviderPK>();
			self.Wait.test_and_set();
			self.Wait.notify_one();
		}(id, *this);
	}

	TEST_F( OpcServerTests, Crud ){
		try{
			CrudImpl();
			Wait.wait( false );
			ASSERT_EQ( 0, std::any_cast<ProviderPK>(Result) );
		}
		catch( const IException& e ){
			ASSERT_DESC( false, e.what() );
		}
	}
}