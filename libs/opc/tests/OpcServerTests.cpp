#include "gtest/gtest.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/db/Database.h"
#include "helpers.h"
#include <jde/opc/types/OpcServer.h>
#include <jde/io/Json.h>
#include <jde/opc/UM.h>
#include <jde/opc/IotGraphQL.h>

#define let const auto
namespace Jde::Opc{
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
		let target = OpcServerTarget;
		json jInsert = Json::Parse( Ƒ("{{\"input\":{{\"target\":\"{}\"}}}}", target) );
		DB::MutationQL insert{ "opcServer", DB::EMutationQL::Create, jInsert, nullopt };

		let existingProviderPK = *( co_await ProviderAwait{target} ).UP<ProviderPK>();
		let existingServer = SelectOpcServer();
		let existingOpcPK = Json::Get<OpcPK>( existingServer, "id" );
		if( !existingOpcPK && !existingProviderPK ){
			[](auto&& insert, auto self)->CreateOpcServerAwait::Task {
				auto pk = co_await CreateOpcServerAwait();
				DB::DataSourcePtr()->Execute(	Ƒ("delete from iot_opc_servers where id='{}'", pk) ); //InsertFailed checks if failure occurs because exists.
				[](auto&& insert, auto self)->Task{
					co_await *GetHook()->InsertFailure( insert, 0 );
					self->Id = *( co_await ProviderAwait{ OpcServerTarget } ).UP<ProviderPK>();
					self->Wait.test_and_set();
					self->Wait.notify_one();
				}( insert, self );
			}( insert, this );
		}
		else{
			if( existingOpcPK )
				DB::DataSourcePtr()->Execute(	Ƒ("delete from iot_opc_servers where id='{}'", existingOpcPK) ); //InsertFailed checks if failure occurs because exists.
			co_await *GetHook()->InsertFailure( insert, 0 );
			Id = *( co_await ProviderAwait{ OpcServerTarget } ).UP<ProviderPK>();
			Wait.test_and_set();
			Wait.notify_one();
		}
	}

	TEST_F( OpcServerTests, InsertFailed ){
		InsertFailedImpl();
		Wait.wait( false );
		ASSERT_EQ( 0, std::any_cast<ProviderPK>(Id) );
	}

	α OpcServerTests::PurgeFailedImpl()ε->CreateOpcServerAwait::Task{
		let existingServer = SelectOpcServer();
		auto opcPK = Json::Get<OpcPK>( existingServer, "id" );
		if( !opcPK )
			opcPK = co_await CreateOpcServerAwait();
		Id = opcPK;
		[](auto opcPK, auto self)->Task {
			co_await ProviderAwait{OpcServerTarget, false};//BeforePurge mock.
			json jPurge = json{ {"id", opcPK} };
			DB::MutationQL purge{ "opcServer", DB::EMutationQL::Purge, jPurge, nullopt };
			co_await *GetHook()->PurgeFailure( purge, 0 );
			self->Result = *( co_await ProviderAwait{OpcServerTarget} ).UP<ProviderPK>();
			self->Wait.test_and_set();
			self->Wait.notify_one();
		}(opcPK, this);
	}
	TEST_F( OpcServerTests, PurgeFailed ){
		PurgeFailedImpl();
		Wait.wait( false );
		ASSERT_NE( 0, std::any_cast<ProviderPK>(Result) );
		PurgeOpcServer( std::any_cast<OpcPK>(Id) );
	}

	α OpcServerTests::CrudImpl()ε->CreateOpcServerAwait::Task{
		let existingServer = SelectOpcServer();
		let existingOpcPK = Json::Get<OpcPK>( existingServer, "id" );
		if( existingOpcPK )
			PurgeOpcServer( existingOpcPK );
		let createdId = co_await CreateOpcServerAwait();
		let selectAll = "{ query opcServers{ id name attributes created updated deleted target description certificateUri isDefault url } }";
		let selectAllJson = DB::Query( selectAll, 0 );
		Trace( _tags, "selectAllJson={}", selectAllJson.dump() );
		let id = selectAllJson["data"]["opcServers"][0]["id"].get<OpcPK>();
		THROW_IF( createdId!=id, "createdId={} id={}", createdId, id );
		CrudImpl2( id );
	}

	α OpcServerTests::CrudImpl2( OpcPK id )ε->Task{
		auto readJson = SelectOpcServer();
		THROW_IF( Json::Getε<OpcPK>(readJson, "id")!=id, "id={} readJson={}", id, readJson.dump() );
		let target = Json::Getε( readJson, "target" );

		let providerId = *( co_await ProviderAwait{target} ).UP<ProviderPK>();
		THROW_IF( providerId==0, "providerId==0" );;

		let description = "new description";
		let update = Ƒ( "{{ mutation updateOpcServer( 'id':{}, 'input': {{'description':'{}'}} ) }}", id, description );
		let updateJson = DB::Query( Str::Replace(update, '\'', '"'), 0 );
		Trace( _tags, "updateJson={}", updateJson.dump() );
		let updated = SelectOpcServer( id );
		THROW_IF( Json::Getε( updated, "description" )!=description, "description={} updated={}", description, updated.dump() );

		let del = Ƒ( "{{mutation deleteOpcServer('id':{}) }}", id );
		let deleteJson = DB::Query( Str::Replace(del, '\'', '"'), 0 );
		Trace( _tags, "deleted={}", deleteJson.dump() );
	 	auto readJson2 = SelectOpcServer( id );
		THROW_IF( readJson2["deleted"].is_null(), "deleted failed" );

		CrudPurge( id );
	}

	α OpcServerTests::CrudPurge( OpcPK id )ε->PurgeOpcServerAwait::Task{
		co_await PurgeOpcServerAwait{ id };
		[]( OpcPK id, OpcServerTests& self )->Task {
			let opcServer = ( co_await OpcServer::Select(id, true) ).UP<OpcServer>();
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