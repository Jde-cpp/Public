#include <jde/access/Authorize.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/web/server/Sessions.h>
#include "../../AppServer/src/LocalClient.h"
#include "../../OpcGateway/src/GatewayAppClient.h"
#define let const auto

//The in-process app client (src/HubAppClient.h): every socket round trip the gateway makes to an AppServer answered here.
namespace Jde::Opc::Hub::Tests{
	using Web::FromServer::SessionInfo;
	constexpr sv Endpoint{ "127.0.0.1" };

	struct HubClientTests : ::testing::Test{
		Ω CreateProvider( str target )ε->Access::ProviderPK{
			let j = Gateway::AppClient()->QuerySync<jobject>( Ƒ("createProvider( target:\"{}\", providerType:\"OpcServer\" ){{id}}", target), {} );
			return QL::AsId<Access::ProviderPK>( j );
		}
		Ω FindProvider( str target )ε->Access::ProviderPK{
			let j = Gateway::AppClient()->QuerySync<jobject>( "provider(name:$opcTarget){ id }", {{"opcTarget", target}} );
			return Json::FindNumber<Access::ProviderPK>( j, "id" ).value_or( 0 );
		}
		Ω PurgeProvider( Access::ProviderPK pk )ε->void{
			Gateway::AppClient()->QuerySync<jvalue>( "purgeProvider( id:$id )", {{"id", pk}} );
		}
	};

	//the seam: the gateway's client is local, shares the AppServer's QL, Authorize, registration pks and signing key.
	TEST_F( HubClientTests, Seam ){
		auto client = Gateway::AppClient();
		auto appServer = App::Server::AppClient();
		EXPECT_TRUE( client->IsLocal() );
		EXPECT_TRUE( client->Connected() );
		EXPECT_TRUE( client->UserPK().Value==Jde::UserPK::System );//==, not EXPECT_EQ: gtest takes its operands by reference, which odr-uses the constexpr member.
		EXPECT_EQ( client->QLServer().get(), static_cast<QL::IQL*>(App::Server::QLPtr().get()) );
		EXPECT_EQ( client->Acl().get(), App::Server::Authorizer().get() );
		EXPECT_NE( client->InstancePK(), 0u );
		EXPECT_EQ( client->InstancePK(), appServer->InstancePK() );
		EXPECT_EQ( client->ConnectionPK(), appServer->ConnectionPK() );
		EXPECT_TRUE( client->PublicKey()==appServer->PublicKey() );
	}

	//GraphQL to the app db (the provider lookups the OPC login path makes) through the local QL, as System.
	TEST_F( HubClientTests, ProviderRoundTrip ){
		let target = "hubTestsProvider";
		if( let existing = FindProvider(target); existing )
			PurgeProvider( existing );
		let pk = CreateProvider( target );
		ASSERT_NE( pk, 0u );
		EXPECT_EQ( FindProvider(target), pk );
		PurgeProvider( pk );
		EXPECT_EQ( FindProvider(target), 0u );
	}

	//OPC user/password login's AddSession: authenticate + mint a web session in-process (the kAddSession round trip).
	TEST_F( HubClientTests, AddSession ){
		let target = "hubTestsAddSession";
		if( let existing = FindProvider(target); existing )
			PurgeProvider( existing );
		let providerPK = CreateProvider( target );
		ASSERT_NE( providerPK, 0u );
		auto await = Gateway::AppClient()->AddSession( target, "hubTestsUser", providerPK, string{Endpoint}, false );
		let info = BlockAwait<TAwait<SessionInfo>,SessionInfo>( move(*await) );
		EXPECT_NE( info.session_id(), 0u );
		EXPECT_NE( info.user_pk(), 0u );
		auto session = Web::Server::Sessions::Find( info.session_id() );
		ASSERT_TRUE( session );
		EXPECT_EQ( session->UserPK.Value, info.user_pk() );
		EXPECT_EQ( session->UserEndpoint, Endpoint );
		Web::Server::Sessions::Remove( info.session_id() );
		PurgeProvider( providerPK );
	}
}
