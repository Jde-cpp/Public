#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/client/http/ClientHttpResException.h>
#include <jde/web/server/Sessions.h>
#include "../../AppServer/src/LocalClient.h"
#include "../../OpcGateway/src/GatewayAppClient.h"
#define let const auto

//One listener for both roles (src/HttpRequestAwait.cpp, src/ql/HubQL.cpp): the AppServer's and the gateway's REST routes, one
///graphql over access+app+gateway, one session table and one live logger.  AppPort()==GatewayPort() - the split names stay so
//the two apps' helpers keep working.  Plain http - the server detects ssl per connection.
namespace Jde::Opc::Hub::Tests{
	using Web::Client::ClientHttpAwait; using Web::Client::ClientHttpRes; using Web::Client::ClientHttpResException;
	namespace http = boost::beast::http;
	constexpr sv Host{ "127.0.0.1" };//sessions are endpoint-bound; connect by ip so the socket's remote address matches the endpoint sessions are minted with.
	Ξ AppPort()ι->PortType{ return Settings::FindNumber<PortType>("/http/app/port").value_or(1973); }
	using Gateway::Tests::GatewayPort;

	struct HubRoutingTests : ::testing::Test{
		Ω Get( PortType port, string target, string authorization={} )->ClientHttpRes{
			return BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{string{Host}, move(target), port, {.Authorization=move(authorization), .IsSsl=false}} );
		}
		Ω Post( PortType port, string target, string body, string authorization={} )->ClientHttpRes{
			return BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{string{Host}, move(target), move(body), port, {.Authorization=move(authorization), .ContentType="application/json", .IsSsl=false}} );
		}
		Ω QL( PortType port, string query, string authorization={} )->jvalue{//`data`: an object for a query, an array for a mutation.
			let res = Post( port, "/graphql", serialize(jobject{{"query", move(query)}}), move(authorization) );
			let json = res.Json();
			return json.at( "data" );
		}
	};

	TEST_F( HubRoutingTests, BothRestSurfaces ){
		let clientId = Get( AppPort(), "/GoogleAuthClientId" );
		EXPECT_EQ( Json::AsString(clientId.Json(), "value"), "opc-hub-tests-google-client-id" );
		let errorCodes = Get( GatewayPort(), "/ErrorCodes?scs=2150891520" );//BadCertificateUntrusted
		EXPECT_EQ( Json::AsArray(errorCodes.Json(), "errorCodes").size(), 1u );
	}

	//what the SPA discovers: the gateway role of this process, at the one port - a local registration, no socket.
	TEST_F( HubRoutingTests, OpcGatewaysListsSelf ){
		let res = Get( AppPort(), "/opcGateways" );
		let json = res.Json();//Json() builds a value - hold it, the array below is a reference into it.
		let& servers = Json::AsArray( json, "servers" );
		ASSERT_GE( servers.size(), 1u ) << serialize( json );
		let& self = servers[0].as_object();
		EXPECT_EQ( Json::AsNumber<PortType>(self, "port"), GatewayPort() );
		EXPECT_EQ( Json::AsString(self, "host"), Settings::FindString("/http/host").value_or(Process::HostName()) );
		EXPECT_EQ( Json::AsString(self, "instanceName"), *Settings::FindString("/instanceName") );
	}

	//one process, one registration: a Tests.OpcHub row and no separate gateway row.
	TEST_F( HubRoutingTests, OneConnectionRow ){
		let data = QL( AppPort(), "connections{ id programName instanceName instanceId }" ).as_object();
		uint hub{}, gateway{};
		for( let& c : Json::AsArray(data, "connections") ){
			let program = Json::AsString( c.as_object(), "programName" );
			hub += program==Process::AppName();
			gateway += program=="Jde.OpcGateway";
		}
		EXPECT_EQ( hub, 1u ) << serialize( data );
		EXPECT_EQ( gateway, 0u ) << serialize( data );
	}

	//one /graphql over the three schemas: the gateway's, the app's and access' tables, the merged custom queries and the
	//merged status - and access tables stay off the gateway's ConnectAwait (GatewayQLAwait::IsApplicable).
	TEST_F( HubRoutingTests, MergedGraphQL ){
		let sessionId = Web::Server::Sessions::Add( Jde::UserPK{1}, string{Host}, false )->SessionId;
		let authorization = Ƒ( "{:x}", sessionId );
		EXPECT_TRUE( QL(AppPort(), "serverConnections{ id target url }").as_object().contains("serverConnections") );
		EXPECT_TRUE( QL(AppPort(), "connections{ id programName }").as_object().contains("connections") );
		EXPECT_TRUE( QL(AppPort(), "users{ id name }", authorization).as_object().contains("users") );
		let status = Json::AsObject( QL(AppPort(), "status{ memory clients monitoredItems }", authorization).as_object(), "status" );
		EXPECT_TRUE( status.contains("memory") && status.contains("clients") && status.contains("monitoredItems") ) << serialize( status );
		let type = QL( AppPort(), "__type(name:\"ServerConnection\"){ name fields{ name } }" ).as_object();
		EXPECT_TRUE( type.contains("__type") ) << serialize( type );
		Web::Server::Sessions::Remove( sessionId );
	}

	//POST /login by shape: a Bearer JWT is the app's, a JSON body with `opc` the gateway's OPC login, neither is the app's 401.
	TEST_F( HubRoutingTests, LoginByShape ){
		auto failure = [&]( function<void()> call )->optional<http::status>{
			try{ call(); }
			catch( ClientHttpResException& e ){ return e.Status(); }
			catch( const std::exception& ){}
			return {};
		};
		EXPECT_EQ( failure([&]{ Post(AppPort(), "/login", "{}"); }), http::status::unauthorized );
		EXPECT_NE( failure([&]{ Post(AppPort(), "/login", "{}", "Bearer not-a-jwt"); }), optional<http::status>{} );
		let opc = failure( [&]{ Post(AppPort(), "/login", serialize(jobject{{"opc","noSuchServer"},{"user","u"},{"password","p"}})); } );
		EXPECT_TRUE( opc.has_value() && *opc!=http::status::not_found ) << "the opc login was not routed to the gateway.";
	}

	//POST /logout ends the web session (and so both protocols' sockets) - the gateway's and the app's logout in one.
	TEST_F( HubRoutingTests, LogoutRemovesSession ){
		let sessionId = Web::Server::Sessions::Add( Jde::UserPK{1}, string{Host}, false )->SessionId;
		let authorization = Ƒ( "{:x}", sessionId );
		let res = Post( AppPort(), "/logout", "{}", authorization );
		EXPECT_TRUE( Json::AsBool(res.Json(), "removed") );
		EXPECT_FALSE( Web::Server::Sessions::Find(sessionId) );
	}

	//a session minted through the AppServer role is honoured by the gateway role without a lookup - one table, IsLocal.
	TEST_F( HubRoutingTests, SessionShared ){
		let sessionId = Web::Server::Sessions::Add( Jde::UserPK{1}, string{Host}, false )->SessionId;
		let authorization = Ƒ( "{:x}", sessionId );
		let data = QL( GatewayPort(), "status{ memory }", authorization ).as_object();
		EXPECT_TRUE( Json::AsObject(data, "status").contains("memory") ) << serialize( data );
		EXPECT_TRUE( Web::Server::Sessions::Remove(sessionId) );
		EXPECT_THROW( QL(GatewayPort(), "status{ memory }", authorization), ClientHttpResException );//revoked on one surface, gone on the other.
	}

	//a level set through the AppServer role's mutation is read straight back from the live logger through the gateway role
	//(logSetting{} wants an authenticated caller - a session minted in-process, as the SPA's would be after login).
	TEST_F( HubRoutingTests, LogLevelRoundTrip ){
		let instance = Gateway::AppClient()->InstancePK();
		ASSERT_NE( instance, 0u );
		let sessionId = Web::Server::Sessions::Add( Jde::UserPK{1}, string{Host}, false )->SessionId;
		let authorization = Ƒ( "{:x}", sessionId );
		auto level = [&]()->string{
			let data = QL( GatewayPort(), "logSetting{ text }", authorization ).as_object();
			let& text = Json::AsObject( Json::AsObject(data, "logSetting"), "text" );
			return text.contains("test") ? string{ text.at("test").as_string() } : string{};
		};
		QL( AppPort(), Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"text\":[{{tags:[\"test\"],level:\"Critical\"}}] )", instance), authorization );
		EXPECT_EQ( level(), "Critical" );
		QL( AppPort(), Ƒ("mutation updateInstanceTagLevel( \"id\":{}, \"text\":[{{tags:[\"test\"],level:null}}] )", instance), authorization );
		EXPECT_NE( level(), "Critical" );
		Web::Server::Sessions::Remove( sessionId );
	}
}
