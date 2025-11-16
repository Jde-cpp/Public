//#include "gtest/gtest.h"
#include <jde/fwk/io/json.h>
#include <jde/db/IDataSource.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include "../src/StartupAwait.h"
#include "../src/opcInternal.h"
#include "../src/auth/UM.h"
#include "../src/ql/OpcQLHook.h"
#include "utils/helpers.h"

#define let const auto
namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	using Gateway::QL;

	struct ServerCnnctnDBTests : ::testing::Test{
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
		α CrudImpl2( ServerCnnctnPK id )ε->Access::ProviderPK;
		α CrudPurge( ServerCnnctnPK id )ε->Access::ProviderPK;
	};
	uint ServerCnnctnDBTests::OpcProviderId{};

	α GetProviderPK( string target )ε->Access::ProviderPK{
		return BlockAwait<ProviderSelectAwait,Access::ProviderPK>( ProviderSelectAwait{target} );
	}
	α GetOpcServers( optional<DB::Key> key=nullopt, bool includeDeleted=false )->vector<ServerCnnctn>{
		return BlockAwait<ServerCnnctnAwait,vector<ServerCnnctn>>( ServerCnnctnAwait{key, includeDeleted} );
	}

	TEST_F( ServerCnnctnDBTests, InsertFailed ){
		TRACE( "InsertFailed::Started" );
		let target = OpcServerTarget;
		auto jInsert = Json::Parse( Ƒ("{{\"target\":\"{}\"}}", target) );
		QL::MutationQL insert{ "createServerConnection", move(jInsert), {}, nullopt, true, QL().Schemas(), false };

		let existingProviderPK = GetProviderPK( target );
		let existingServer = SelectServerCnnctn( target );
		let existingOpcPK = existingServer ? existingServer->Id : 0;
		let& table = GetViewPtr( "server_connections" );
		if( !existingOpcPK && !existingProviderPK ){
			auto pk = BlockAwait<CreateServerCnnctnAwait,ServerCnnctnPK>( CreateServerCnnctnAwait{} );
			DS()->ExecuteSync( {Ƒ("delete from {} where server_connection_id='{}'", table->DBName, pk)} ); //InsertFailed checks if failure occurs because exists.
		}
		else{
			if( existingOpcPK )
				DS()->ExecuteSync( {Ƒ("delete from {} where server_connection_id='{}'", table->DBName, existingOpcPK)} ); //InsertFailed checks if failure occurs because exists.
		}
		BlockAwait<TAwait<jvalue>,jvalue>( move(*OpcQLHook{}.InsertFailure(insert, {UserPK::System})) );
		ASSERT_EQ( 0, GetProviderPK(target) );
	}

	TEST_F( ServerCnnctnDBTests, PurgeFailed ){
		let existingServer = SelectServerCnnctn( OpcServerTarget );
		auto opcPK = existingServer ? existingServer->Id : 0;
		if( !opcPK )
			opcPK = BlockAwait<CreateServerCnnctnAwait,ServerCnnctnPK>( CreateServerCnnctnAwait{} );
		Id = opcPK;
		BlockAwait<ProviderCreatePurgeAwait,Access::ProviderPK>( ProviderCreatePurgeAwait{OpcServerTarget, false} );//BeforePurge mock.

		QL::MutationQL purge{ "purgeServerConnection", { {"id", opcPK} }, {}, nullopt, true, QL().Schemas(), false };
		BlockAwait<TAwait<jvalue>,jvalue>( move(*OpcQLHook{}.PurgeFailure(purge, {UserPK::System})) );
		let providerPK = GetProviderPK( OpcServerTarget );
		ASSERT_NE( 0, providerPK );
		PurgeServerCnnctn();
	}

	α ServerCnnctnDBTests::CrudImpl()ε->Access::ProviderPK{
		let existingServer = SelectServerCnnctn( OpcServerTarget );
		let existingOpcPK = existingServer ? existingServer->Id : 0;
		if( existingOpcPK )
			PurgeServerCnnctn( existingOpcPK );
		let createdId = BlockAwait<CreateServerCnnctnAwait,ServerCnnctnPK>( CreateServerCnnctnAwait{} );
		let selectAll = "serverConnections{ id name attributes created updated deleted target description certificateUri isDefault url }";
		let selectAllJson = QL().QuerySync<jarray>( selectAll, {UserPK::System} );
		TRACET( _tags, "selectAllJson={}", serialize(selectAllJson) );
		let id = Json::AsNumber<ServerCnnctnPK>( Json::AsObject(selectAllJson[0]), "id" );
		THROW_IF( createdId!=id, "createdId={} id={}", createdId, id );
		return CrudImpl2( id );
	}

	α ServerCnnctnDBTests::CrudImpl2( ServerCnnctnPK id )ε->Access::ProviderPK{
		auto conn = SelectServerCnnctn( OpcServerTarget );
		THROW_IF( conn->Id!=id, "id={} readJson={}", id, serialize(conn->ToJson()) );
		let target = conn->Target;

		let providerId = BlockAwait<ProviderSelectAwait,Access::ProviderPK>( ProviderSelectAwait{target} );
		THROW_IF( providerId==0, "providerId==0" );

		let description = "new description";
		let update = Ƒ( "mutation updateServerConnection( id:{}, description:\"{}\" ) }}", id, description );
		let updateJson = QL().QuerySync<jvalue>( update, {UserPK::System} );
		TRACET( _tags, "updateJson={}", serialize(updateJson) );
		let updated = SelectServerCnnctn( id );
		THROW_IF( updated->Description!=description, "description={} updated={}", description, serialize(updated->ToJson()) );

		let del = Ƒ( "deleteServerConnection(\"id\":{})", id );
		let deleteJson = QL().QuerySync<jvalue>( del, {UserPK::System} );
		TRACET( _tags, "deleted={}", serialize(deleteJson) );
	 	conn = SelectServerCnnctn( id );
		THROW_IF( !conn->Deleted, "deleted failed" );

		return CrudPurge( id );
	}

	α ServerCnnctnDBTests::CrudPurge( ServerCnnctnPK id )ε->Access::ProviderPK{
		BlockAwait<PurgeServerCnnctnAwait,uint>( PurgeServerCnnctnAwait{ id } );
		let opcServers = GetOpcServers( id, true );
		THROW_IF( opcServers.size(), "Purge Failed" );
		return GetProviderPK( OpcServerTarget );
	}

	TEST_F( ServerCnnctnDBTests, Crud ){
		try{
			auto providerPK = CrudImpl();
			ASSERT_EQ( 0, providerPK );
		}
		catch( const IException& e ){
			ASSERT_DESC( false, e.what() );
		}
	}
}