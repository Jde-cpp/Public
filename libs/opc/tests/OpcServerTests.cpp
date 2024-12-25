#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include <jde/opc/types/OpcServer.h>
#include <jde/opc/UM.h>
#include <jde/opc/OpcQLHook.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>
#include "helpers.h"

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
		α InsertFailedImpl()ε->CreateOpcServerAwait::Task;
		α PurgeFailedImpl()ε->CreateOpcServerAwait::Task;
		α CrudImpl()ε->CreateOpcServerAwait::Task;
		α CrudImpl2( OpcPK id )ε->ProviderSelectAwait::Task;
		α CrudPurge( OpcPK id )ε->PurgeOpcServerAwait::Task;
	};
	uint OpcServerTests::OpcProviderId{};

	α GetProviderPK( string target )ε->Access::ProviderPK{
		return BlockAwait<ProviderSelectAwait,Access::ProviderPK>( ProviderSelectAwait{target} );
	}
	α GetOpcServers( optional<DB::Key> key=nullopt, bool includeDeleted=false )->vector<OpcServer>{
		return BlockAwait<OpcServerAwait,vector<OpcServer>>( OpcServerAwait{} );
	}

	α OpcServerTests::InsertFailedImpl()ε->CreateOpcServerAwait::Task{
		let target = OpcServerTarget;
		let jInsert = Json::Parse( Ƒ("{{\"input\":{{\"target\":\"{}\"}}}}", target) );
		QL::MutationQL insert{ "opcServer", QL::EMutationQL::Create, jInsert, nullopt };

		let existingProviderPK = GetProviderPK( target);
		let existingServer = SelectOpcServer();
		let existingOpcPK = Json::AsNumber<OpcPK>( existingServer, "id" );
		let& table = GetViewPtr( "opc_servers" );
		if( !existingOpcPK && !existingProviderPK ){
			auto pk = co_await CreateOpcServerAwait();
			DS().Execute(	Ƒ("delete from {} where id='{}'", table->DBName, pk) ); //InsertFailed checks if failure occurs because exists.
			[&](auto&& insert, auto self)->TAwait<jvalue>::Task {
				co_await *GetHook()->InsertFailure( insert, {UserPK::System} );
				self->Id = GetProviderPK( target );
				self->Wait.test_and_set();
				self->Wait.notify_one();
			}( insert, this );
		}
		else{
			if( existingOpcPK )
				DS().Execute(	Ƒ("delete from {} where id='{}'", table->DBName, existingOpcPK) ); //InsertFailed checks if failure occurs because exists.
			[=,this](auto&& insert, auto self)->TAwait<jvalue>::Task {
				co_await *GetHook()->InsertFailure( insert, {UserPK::System} );
				Id = GetProviderPK( target );
				Wait.test_and_set();
				Wait.notify_one();
			}( insert, this );
		}
	}

	TEST_F( OpcServerTests, InsertFailed ){
		InsertFailedImpl();
		Wait.wait( false );
		ASSERT_EQ( 0, std::any_cast<Access::ProviderPK>(Id) );
	}

	α OpcServerTests::PurgeFailedImpl()ε->CreateOpcServerAwait::Task{
		let existingServer = SelectOpcServer();
		auto opcPK = Json::AsNumber<OpcPK>( existingServer, "id" );
		if( !opcPK )
			opcPK = co_await CreateOpcServerAwait();
		Id = opcPK;
		[](auto opcPK, auto self)->ProviderCreatePurgeAwait::Task {
			co_await ProviderCreatePurgeAwait{OpcServerTarget, false};//BeforePurge mock.
			[](auto opcPK, auto self)->TAwait<jvalue>::Task {
				QL::MutationQL purge{ "opcServer", QL::EMutationQL::Purge, { {"id", opcPK} }, nullopt };
				co_await *GetHook()->PurgeFailure( purge, {UserPK::System} );
				self->Result = GetProviderPK( OpcServerTarget );
				self->Wait.test_and_set();
				self->Wait.notify_one();
			}(opcPK, self);
		}(opcPK, this);
	}
	TEST_F( OpcServerTests, PurgeFailed ){
		PurgeFailedImpl();
		Wait.wait( false );
		ASSERT_NE( 0, std::any_cast<Access::ProviderPK>(Result) );
		PurgeOpcServer( std::any_cast<OpcPK>(Id) );
	}

	α OpcServerTests::CrudImpl()ε->CreateOpcServerAwait::Task{
		let existingServer = SelectOpcServer();
		let existingOpcPK = Json::AsNumber<OpcPK>( existingServer, "id" );
		if( existingOpcPK )
			PurgeOpcServer( existingOpcPK );
		let createdId = co_await CreateOpcServerAwait();
		let selectAll = "opcServers{ id name attributes created updated deleted target description certificateUri isDefault url }";
		let selectAllJson = QL::QueryArray( selectAll, {UserPK::System} );
		Trace( _tags, "selectAllJson={}", serialize(selectAllJson) );
		let id = Json::AsNumber<OpcPK>( Json::AsObject(selectAllJson[0]), "id" );
		THROW_IF( createdId!=id, "createdId={} id={}", createdId, id );
		CrudImpl2( id );
	}

	α OpcServerTests::CrudImpl2( OpcPK id )ε->ProviderSelectAwait::Task{
		auto readJson = SelectOpcServer();
		THROW_IF( Json::AsNumber<OpcPK>(readJson, "id")!=id, "id={} readJson={}", id, serialize(readJson) );
		let target = Json::AsString( readJson, "target" );

		let providerId = co_await ProviderSelectAwait{target};
		THROW_IF( providerId==0, "providerId==0" );;

		let description = "new description";
		let update = Ƒ( "{{ mutation updateOpcServer( 'id':{}, 'input': {{'description':'{}'}} ) }}", id, description );
		let updateJson = QL::Query( Str::Replace(update, '\'', '"'), {UserPK::System} );
		Trace( _tags, "updateJson={}", serialize(updateJson) );
		let updated = SelectOpcServer( id );
		THROW_IF( Json::AsString( updated, "description" )!=description, "description={} updated={}", description, serialize(updated) );

		let del = Ƒ( "{{mutation deleteOpcServer('id':{}) }}", id );
		let deleteJson = QL::Query( Str::Replace(del, '\'', '"'), {UserPK::System} );
		Trace( _tags, "deleted={}", serialize(deleteJson) );
	 	auto readJson2 = SelectOpcServer( id );
		THROW_IF( readJson2["deleted"].is_null(), "deleted failed" );

		CrudPurge( id );
	}

	α OpcServerTests::CrudPurge( OpcPK id )ε->PurgeOpcServerAwait::Task{
		co_await PurgeOpcServerAwait{ id };
		let opcServers = GetOpcServers( id, true );
		THROW_IF( opcServers.size(), "Purge Failed" );
		Result = GetProviderPK( OpcServerTarget );
		Wait.test_and_set();
		Wait.notify_one();
	}

	TEST_F( OpcServerTests, Crud ){
		try{
			CrudImpl();
			Wait.wait( false );
			ASSERT_EQ( 0, std::any_cast<Access::ProviderPK>(Result) );
		}
		catch( const IException& e ){
			ASSERT_DESC( false, e.what() );
		}
	}
}