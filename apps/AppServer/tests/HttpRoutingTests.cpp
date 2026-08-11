#include <jde/web/client/http/ClientHttpAwait.h>
#include <jde/web/client/http/ClientHttpResException.h>
#include "helpers.h"
#define let const auto

//the REST surface: HttpRequestAwait routing (/GoogleAuthClientId, /opcGateways, /opcServers, /login, /logout, 404s)
//plus the /graphql dispatch in Web::Server.  All plain http - the server detects ssl per connection.
namespace Jde::App::Server::Tests{
	using Web::Client::ClientHttpAwait; using Web::Client::ClientHttpRes; using Web::Client::ClientHttpResException;
	namespace http = boost::beast::http;

	struct HttpRoutingTests : ::testing::Test{
		Ω Get( string target, string authorization={} )->ClientHttpRes{
			return BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{string{Host}, move(target), Port(), {.Authorization=move(authorization), .IsSsl=false}} );
		}
		Ω Post( string target, string body, string authorization={}, string contentType="application/json" )->ClientHttpRes{
			return BlockAwait<ClientHttpAwait,ClientHttpRes>( ClientHttpAwait{string{Host}, move(target), move(body), Port(), {.Authorization=move(authorization), .ContentType=move(contentType), .IsSsl=false}} );
		}
		//the thrown status for calls that must fail; unset if no ClientHttpResException was thrown.
		Ŧ FailureStatus( T&& call )->optional<http::status>{
			try{
				call();
			}
			catch( ClientHttpResException& e ){
				return e.Status();
			}
			catch( const std::exception& ){
			}
			return {};
		}
	};

	TEST_F( HttpRoutingTests, GoogleAuthClientId ){
		let res = Get( "/GoogleAuthClientId" );
		EXPECT_EQ( Json::AsString(res.Json(), "value"), "app-server-tests-google-client-id" );//the /http/clientSettings pointer - the old unrooted key always answered "Not Configured".
	}

	TEST_F( HttpRoutingTests, OpcGatewaysListsInstances ){
		auto client = Connect();
		RegisterInstance( *client, "Jde.OpcGateway", "http-gw", "gw-host", 4321, 4242 );
		let res = Get( "/opcGateways" );
		let servers = Json::AsArray( res.Json(), "servers" );
		auto found = false;
		for( let& jserver : servers ){
			let& o = jserver.as_object();
			if( Json::AsString(o, "instanceName")!="http-gw" )
				continue;
			found = true;
			EXPECT_EQ( Json::AsString(o, "application"), "Jde.OpcGateway" );
			EXPECT_EQ( Json::AsString(o, "host"), "gw-host" );
			EXPECT_EQ( o.at("port").to_number<uint32>(), 4321u );
			EXPECT_EQ( o.at("pid").to_number<uint32>(), 4242u );
			EXPECT_TRUE( o.contains("startTime") );
		}
		EXPECT_TRUE( found );
		BlockVoidAwait( client->Close(true, SRCE_CUR) );
	}

	TEST_F( HttpRoutingTests, OpcServersListsInstances ){
		auto client = Connect();
		RegisterInstance( *client, "Jde.OpcServer", "http-opc", "opc-host", 4322 );
		let res = Get( "/opcServers" );
		let servers = Json::AsArray( res.Json(), "servers" );
		EXPECT_TRUE( std::ranges::any_of(servers, [](let& s){ return Json::AsString(s.as_object(), "instanceName")=="http-opc"; }) );
		BlockVoidAwait( client->Close(true, SRCE_CUR) );
	}

	TEST_F( HttpRoutingTests, LoginRequiresBearer ){
		EXPECT_EQ( FailureStatus([&]{ Post("/login", "{}"); }), http::status::unauthorized );//no Authorization at all.
		EXPECT_EQ( FailureStatus([&]{ Post("/login", "{}", "Basic dXNlcjpwd2Q="); }), http::status::unauthorized );//wrong scheme.
	}

	TEST_F( HttpRoutingTests, LoginRejectsGarbageJwt ){
		let status = FailureStatus( [&]{ Post("/login", "{}", "Bearer not-a-jwt"); } );
		ASSERT_TRUE( status );
		EXPECT_NE( *status, http::status::ok );
	}

	TEST_F( HttpRoutingTests, LogoutRemovesSession ){
		let sessionId = Web::Server::Sessions::Add( Jde::UserPK{1}, string{Host}, false )->SessionId;
		let res = Post( "/logout", "{}", Ƒ("{:x}", sessionId) );
		EXPECT_TRUE( Json::AsBool(res.Json(), "removed") );
		EXPECT_FALSE( Web::Server::Sessions::Find(sessionId) );
		EXPECT_EQ( FailureStatus([&]{ Post("/logout", "{}", Ƒ("{:x}", sessionId)); }), http::status::unauthorized );//the removed id is no longer a credential.
	}

	TEST_F( HttpRoutingTests, UnknownTarget ){
		EXPECT_EQ( FailureStatus([&]{ Get("/definitely-not-a-route"); }), http::status::not_found );
	}

	TEST_F( HttpRoutingTests, GraphQLPost ){
		let res = Post( "/graphql", serialize(jobject{{"query", "setting(target:\"googleAuthClientId\"){target value}"}}) );
		let data = Json::AsObject( res.Json(), "data" );
		let setting = Json::AsObject( data, "setting" );
		EXPECT_EQ( Json::AsString(setting, "target"), "googleAuthClientId" );
		EXPECT_EQ( Json::AsString(setting, "value"), "app-server-tests-google-client-id" );
	}

	TEST_F( HttpRoutingTests, GraphQLGet ){
		let res = Get( "/graphql?query=setting(target:%22googleAuthClientId%22)%7Btarget%20value%7D" );
		let data = Json::AsObject( res.Json(), "data" );
		EXPECT_EQ( Json::AsString(Json::AsObject(data, "setting"), "value"), "app-server-tests-google-client-id" );
	}

	TEST_F( HttpRoutingTests, GraphQLRequiresQuery ){
		EXPECT_EQ( FailureStatus([&]{ Post("/graphql", "{}"); }), http::status::bad_request );
	}
}
