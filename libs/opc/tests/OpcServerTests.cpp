#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include <jde/opc/types/OpcClient.h>
#include <jde/opc/UM.h>
#include <jde/opc/OpcQLHook.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>
#include "../src/opcInternal.h"
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
		α InsertFailedImpl()ε->Access::ProviderPK;
		α PurgeFailedImpl()ε->Access::ProviderPK;
		α CrudImpl()ε->CreateOpcClientAwait::Task;
		α CrudImpl2( OpcClientPK id )ε->ProviderSelectAwait::Task;
		α CrudPurge( OpcClientPK id )ε->PurgeOpcClientAwait::Task;
	};
	uint OpcServerTests::OpcProviderId{};

	α GetProviderPK( string target )ε->Access::ProviderPK{
		return BlockAwait<ProviderSelectAwait,Access::ProviderPK>( ProviderSelectAwait{target} );
	}
	α GetOpcServers( optional<DB::Key> key=nullopt, bool includeDeleted=false )->vector<OpcClient>{
		return BlockAwait<OpcClientAwait,vector<OpcClient>>( OpcClientAwait{key, includeDeleted} );
	}

	α OpcServerTests::InsertFailedImpl()ε->Access::ProviderPK{
		let target = OpcServerTarget;
		auto jInsert = Json::Parse( Ƒ("{{\"target\":\"{}\"}}", target) );
		QL::MutationQL insert{ "createOpcClient", move(jInsert), nullopt, true };

		let existingProviderPK = GetProviderPK( target );
		let existingServer = SelectOpcClient( target );
		let existingOpcPK = Json::FindNumber<OpcClientPK>( existingServer, "id" ).value_or(0);
		let& table = GetViewPtr( "clients" );
		if( !existingOpcPK && !existingProviderPK ){
			auto pk = BlockAwait<CreateOpcClientAwait,OpcClientPK>( CreateOpcClientAwait{} );
			DS()->ExecuteSync( {Ƒ("delete from {} where client_id='{}'", table->DBName, pk)} ); //InsertFailed checks if failure occurs because exists.
		}
		else{
			if( existingOpcPK )
				DS()->ExecuteSync( {Ƒ("delete from {} where client_id='{}'", table->DBName, existingOpcPK)} ); //InsertFailed checks if failure occurs because exists.
		}
		BlockAwait<TAwait<jvalue>,jvalue>( *GetHook()->InsertFailure(insert, {UserPK::System}) );
		return GetProviderPK( target );
	}

	TEST_F( OpcServerTests, InsertFailed ){
		Trace{ _tags, "InsertFailed::Started" };
		ASSERT_EQ( 0, InsertFailedImpl() );
	}

	α OpcServerTests::PurgeFailedImpl()ε->Access::ProviderPK{
		let existingServer = SelectOpcClient( OpcServerTarget );
		auto opcPK = Json::FindNumber<OpcClientPK>( existingServer, "id" ).value_or(0);
		if( !opcPK )
			opcPK = BlockAwait<CreateOpcClientAwait,OpcClientPK>( CreateOpcClientAwait{} );
		Id = opcPK;
		BlockAwait<ProviderCreatePurgeAwait,Access::ProviderPK>( ProviderCreatePurgeAwait{OpcServerTarget, false} );//BeforePurge mock.

		QL::MutationQL purge{ "purgeOpcClient", { {"id", opcPK} }, nullopt, true };
		BlockAwait<TAwait<jvalue>,jvalue>( *GetHook()->PurgeFailure(purge, {UserPK::System}) );
		return GetProviderPK( OpcServerTarget );
	}
	TEST_F( OpcServerTests, PurgeFailed ){
		let providerPK = PurgeFailedImpl();
		ASSERT_NE( 0, providerPK );
		PurgeOpcClient();
	}

	α OpcServerTests::CrudImpl()ε->CreateOpcClientAwait::Task{
		let existingServer = SelectOpcClient( OpcServerTarget );
		let existingOpcPK = Json::FindNumber<OpcClientPK>( existingServer, "id" ).value_or(0);
		if( existingOpcPK )
			PurgeOpcClient( existingOpcPK );
		let createdId = co_await CreateOpcClientAwait();
		let selectAll = "opcClients{ id name attributes created updated deleted target description certificateUri isDefault url }";
		let selectAllJson = QL::QueryArray( selectAll, {UserPK::System} );
		Trace( _tags, "selectAllJson={}", serialize(selectAllJson) );
		let id = Json::AsNumber<OpcClientPK>( Json::AsObject(selectAllJson[0]), "id" );
		THROW_IF( createdId!=id, "createdId={} id={}", createdId, id );
		CrudImpl2( id );
	}

	α OpcServerTests::CrudImpl2( OpcClientPK id )ε->ProviderSelectAwait::Task{
		auto readJson = SelectOpcClient( OpcServerTarget );
		THROW_IF( Json::AsNumber<OpcClientPK>(readJson, "id")!=id, "id={} readJson={}", id, serialize(readJson) );
		let target = Json::AsString( readJson, "target" );

		let providerId = co_await ProviderSelectAwait{target};
		THROW_IF( providerId==0, "providerId==0" );;

		let description = "new description";
		let update = Ƒ( "mutation updateOpcClient( id:{}, description:\"{}\" ) }}", id, description );
		let updateJson = QL::Query( update, {UserPK::System} );
		Trace( _tags, "updateJson={}", serialize(updateJson) );
		let updated = SelectOpcClient( id );
		THROW_IF( Json::AsString( updated, "description" )!=description, "description={} updated={}", description, serialize(updated) );

		let del = Ƒ( "{{mutation deleteOpcClient('id':{}) }}", id );
		let deleteJson = QL::Query( Str::Replace(del, '\'', '"'), {UserPK::System} );
		Trace( _tags, "deleted={}", serialize(deleteJson) );
	 	auto readJson2 = SelectOpcClient( id );
		THROW_IF( readJson2["deleted"].is_null(), "deleted failed" );

		CrudPurge( id );
	}

	α OpcServerTests::CrudPurge( OpcClientPK id )ε->PurgeOpcClientAwait::Task{
		co_await PurgeOpcClientAwait{ id };
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