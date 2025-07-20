#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include "../src/auth/UM.h"
#include <jde/opc/OpcQLHook.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include "../src/opcInternal.h"
#include "helpers.h"
#include "../src/StartupAwait.h"

#define let const auto
namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	using Gateway::QL;

	struct ClientDBTests : ::testing::Test{
		atomic_flag Wait;
		std::any Result;
		std::any Id;
	protected:
		Ω SetUpTestCase()ι->void{};
		α SetUp()ι->void{ Wait.clear(); }
		static uint OpcProviderId;
		α InsertFailedImpl()ε->Access::ProviderPK;
		α PurgeFailedImpl()ε->Access::ProviderPK;
		α CrudImpl()ε->Access::ProviderPK;
		α CrudImpl2( OpcClientPK id )ε->Access::ProviderPK;
		α CrudPurge( OpcClientPK id )ε->Access::ProviderPK;
	};
	uint ClientDBTests::OpcProviderId{};

	α GetProviderPK( string target )ε->Access::ProviderPK{
		return BlockAwait<ProviderSelectAwait,Access::ProviderPK>( ProviderSelectAwait{target} );
	}
	α GetOpcServers( optional<DB::Key> key=nullopt, bool includeDeleted=false )->vector<OpcClient>{
		return BlockAwait<OpcClientAwait,vector<OpcClient>>( OpcClientAwait{key, includeDeleted} );
	}

	TEST_F( ClientDBTests, InsertFailed ){
		Trace{ _tags, "InsertFailed::Started" };
		let target = OpcServerTarget;
		auto jInsert = Json::Parse( Ƒ("{{\"target\":\"{}\"}}", target) );
		QL::MutationQL insert{ "createClient", move(jInsert), nullopt, true, QL().Schemas() };

		let existingProviderPK = GetProviderPK( target );
		let existingServer = SelectOpcClient( target );
		let existingOpcPK = existingServer ? existingServer->Id : 0;
		let& table = GetViewPtr( "clients" );
		if( !existingOpcPK && !existingProviderPK ){
			auto pk = BlockAwait<CreateOpcClientAwait,OpcClientPK>( CreateOpcClientAwait{} );
			DS()->ExecuteSync( {Ƒ("delete from {} where client_id='{}'", table->DBName, pk)} ); //InsertFailed checks if failure occurs because exists.
		}
		else{
			if( existingOpcPK )
				DS()->ExecuteSync( {Ƒ("delete from {} where client_id='{}'", table->DBName, existingOpcPK)} ); //InsertFailed checks if failure occurs because exists.
		}
		BlockAwait<TAwait<jvalue>,jvalue>( *OpcQLHook{}.InsertFailure(insert, {UserPK::System}) );
		ASSERT_EQ( 0, GetProviderPK(target) );
	}

	TEST_F( ClientDBTests, PurgeFailed ){
		let existingServer = SelectOpcClient( OpcServerTarget );
		auto opcPK = existingServer ? existingServer->Id : 0;
		if( !opcPK )
			opcPK = BlockAwait<CreateOpcClientAwait,OpcClientPK>( CreateOpcClientAwait{} );
		Id = opcPK;
		BlockAwait<ProviderCreatePurgeAwait,Access::ProviderPK>( ProviderCreatePurgeAwait{OpcServerTarget, false} );//BeforePurge mock.

		QL::MutationQL purge{ "purgeClient", { {"id", opcPK} }, nullopt, true, QL().Schemas() };
		BlockAwait<TAwait<jvalue>,jvalue>( *OpcQLHook{}.PurgeFailure(purge, {UserPK::System}) );
		let providerPK = GetProviderPK( OpcServerTarget );
		ASSERT_NE( 0, providerPK );
		PurgeOpcClient();
	}

	α ClientDBTests::CrudImpl()ε->Access::ProviderPK{
		let existingServer = SelectOpcClient( OpcServerTarget );
		let existingOpcPK = existingServer ? existingServer->Id : 0;
		if( existingOpcPK )
			PurgeOpcClient( existingOpcPK );
		let createdId = BlockAwait<CreateOpcClientAwait,OpcClientPK>( CreateOpcClientAwait{} );
		let selectAll = "clients{ id name attributes created updated deleted target description certificateUri isDefault url }";
		let selectAllJson = QL().QuerySync<jarray>( selectAll, {UserPK::System} );
		Trace( _tags, "selectAllJson={}", serialize(selectAllJson) );
		let id = Json::AsNumber<OpcClientPK>( Json::AsObject(selectAllJson[0]), "id" );
		THROW_IF( createdId!=id, "createdId={} id={}", createdId, id );
		return CrudImpl2( id );
	}

	α ClientDBTests::CrudImpl2( OpcClientPK id )ε->Access::ProviderPK{
		auto client = SelectOpcClient( OpcServerTarget );
		THROW_IF( client->Id!=id, "id={} readJson={}", id, serialize(client->ToJson()) );
		let target = client->Target;

		let providerId = BlockAwait<ProviderSelectAwait,Access::ProviderPK>( ProviderSelectAwait{target} );
		THROW_IF( providerId==0, "providerId==0" );

		let description = "new description";
		let update = Ƒ( "mutation updateClient( id:{}, description:\"{}\" ) }}", id, description );
		let updateJson = QL().QuerySync<jvalue>( update, {UserPK::System} );
		Trace( _tags, "updateJson={}", serialize(updateJson) );
		let updated = SelectOpcClient( id );
		THROW_IF( updated->Description!=description, "description={} updated={}", description, serialize(updated->ToJson()) );

		let del = Ƒ( "deleteClient(\"id\":{})", id );
		let deleteJson = QL().QuerySync<jvalue>( del, {UserPK::System} );
		Trace( _tags, "deleted={}", serialize(deleteJson) );
	 	client = SelectOpcClient( id );
		THROW_IF( !client->Deleted, "deleted failed" );

		return CrudPurge( id );
	}

	α ClientDBTests::CrudPurge( OpcClientPK id )ε->Access::ProviderPK{
		BlockAwait<PurgeOpcClientAwait,uint>( PurgeOpcClientAwait{ id } );
		let opcServers = GetOpcServers( id, true );
		THROW_IF( opcServers.size(), "Purge Failed" );
		return GetProviderPK( OpcServerTarget );
	}

	TEST_F( ClientDBTests, Crud ){
		try{
			auto providerPK = CrudImpl();
			ASSERT_EQ( 0, providerPK );
		}
		catch( const IException& e ){
			ASSERT_DESC( false, e.what() );
		}
	}
}