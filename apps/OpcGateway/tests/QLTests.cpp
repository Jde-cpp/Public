#include "utils/GatewayClientSocket.h"
#include "utils/helpers.h"
#include <jde/web/client/proto/Web.FromServer.pb.h>
#include "../src/GatewayAppClient.h"
#include "../src/auth/OpcServerSession.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	struct QLTests : ::testing::Test{
	protected:
		Ω SetUpTestCase()ε->void{ //ε: CreateServerCnnctn throws - under ι gtest never sees it and the whole binary terminates.
			if( !SelectServerCnnctn( OpcServerTarget ) )
				CreateServerCnnctn();
		};
	};

	TEST_F( QLTests, ServerDescriptionTest ){
		let q = "serverDescription( opc: $opc ){ applicationUri productUri applicationName applicationType gatewayServerUri discoveryProfileUri discoveryUrls }";
		const jobject vars{ {"opc", OpcServerTarget} };
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>(	Socket().Query(q, vars, true) );
		//{"applicationUri":"urn:open62541.server.application","productUri":"http://open62541.org","applicationName":"Jde-Cpp OpcServer","applicationType":"Server","gatewayServerUri":"","discoveryProfileUri":"","discoveryUrls":["opc.tcp://workstation25:4840","opc.tcp://127.0.0.1:4840"]}.
		TRACE( "ServerDescription: {}.", serialize(value) );
		let obj = value.as_object();
		ASSERT_TRUE( obj.contains("applicationUri") );
		ASSERT_TRUE( obj.contains("productUri") );
		ASSERT_TRUE( obj.contains("applicationName") );
		ASSERT_TRUE( obj.contains("applicationType") );
		ASSERT_TRUE( obj.contains("gatewayServerUri") );
		ASSERT_TRUE( obj.contains("discoveryProfileUri") );
		ASSERT_TRUE( obj.contains("discoveryUrls") );
	}

	TEST_F( QLTests, securityPolicyUri ){
		let q = "securityPolicyUri( opc: $opc )";
		const jobject vars{ {"opc", OpcServerTarget} };
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>(	Socket().Query(q, vars, true) );
		TRACE( "securityPolicyUri: {}.", serialize(value) );
		ASSERT_TRUE( serialize(value).size() );
	}

	TEST_F( QLTests, securityMode ){
		let q = "securityMode( opc: $opc )";
		const jobject vars{ {"opc", OpcServerTarget} };
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>(	Socket().Query(q, vars, true) );
		TRACE( "securityMode: {}.", serialize(value) );
		ASSERT_TRUE( serialize(value).size() );
	}
	TEST_F( QLTests, opcSessions ){
		const SessionPK sessionId{ 0x0bc5e551 }; //not a live web session: opcSessions reads the credential cache only, so a seeded entry is enough.
		Credential cred{ string{"opcSessionsTestToken"} }; cred.SetUserPK( AppClient()->UserPK() );
		AddSession( sessionId, OpcServerTarget, move(cred) );
		let q = "opcSessions{ connection{target} type user{id target name} count }";
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query(q, {}, true) );
		TRACE( "opcSessions: {}.", serialize(value) );
		let projected = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query("opcSessions{ count }", {}, true) );
		Logout( sessionId );

		let& rows = value.as_array();
		let userPK = AppClient()->UserPK().Value;
		auto row = find_if( rows, [&](let& r){
			let& o = r.as_object();
			return Json::AsSVPath(o, "connection/target")==OpcServerTarget && Json::AsSV(o, "type")=="IssuedToken" && Json::FindNumberPath<Jde::UserPK::Type>(o, "user/id")==userPK; //user may be null for anonymous rows.
		} );
		ASSERT_NE( row, rows.end() ) << serialize( value );
		let& user = Json::AsObject( row->as_object(), "user" );
		EXPECT_TRUE( user.contains("name") && user.contains("target") ) << serialize( user ); //fetched from AppServer's users table.
		EXPECT_GE( Json::AsNumber<uint32>(row->as_object(), "count"), 1u );

		ASSERT_FALSE( projected.as_array().empty() );
		for( let& r : projected.as_array() ){
			let& o = r.as_object();
			EXPECT_TRUE( o.contains("count") && !o.contains("user") && !o.contains("connection") && !o.contains("type") ) << serialize( o );
		}
	}

	TEST_F( QLTests, webSessionCounted ){ //no manual AddSession: a jwt-backed web session's connect must register itself (ConnectAwait::await_resume).
		const jobject vars{ {"opc", OpcServerTarget} };
		BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query("serverDescription( opc: $opc ){ applicationUri }", vars, true) );//forces a ConnectAwait for this socket's session.
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query("opcSessions{ connection{target} type user{id} count }", {}, true) );
		TRACE( "webSessionCounted: {}.", serialize(value) );
		let userPK = AppClient()->UserPK().Value;
		let& rows = value.as_array();
		let row = find_if( rows, [&](let& r){
			let& o = r.as_object();
			return Json::AsSVPath(o, "connection/target")==OpcServerTarget && Json::AsSV(o, "type")=="IssuedToken" && Json::FindNumberPath<Jde::UserPK::Type>(o, "user/id")==userPK;
		} );
		ASSERT_NE( row, rows.end() ) << serialize( value );
		EXPECT_GE( Json::AsNumber<uint32>(row->as_object(), "count"), 1u );
	}

	TEST_F( QLTests, serverConnectionSessions ){
		const SessionPK sessionId{ 0x0bc5e552 };
		Credential cred{ string{"serverConnectionSessionsTestToken"} }; cred.SetUserPK( AppClient()->UserPK() );
		AddSession( sessionId, OpcServerTarget, move(cred) );
		BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query("serverDescription( opc: $opcTarget ){ applicationUri }", {{"opcTarget", OpcServerTarget}}, true) );//ensure a live UAClient so opcConnections has something to count.
		//the exact shape View.query() emits for the Connections list - args must survive the graft's DB pass.
		let listQL = "serverConnections(limit:25,orderBy:[{name:\"asc\"}],deleted:$deleted){ id name certificateUri url opcSessions{count} opcConnections{count} description target }";
		let rows = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query(listQL, {{"deleted",nullptr}}, true) );
		TRACE( "serverConnections: {}.", serialize(rows) );
		const jobject vars{ {"opc", OpcServerTarget} };
		let single = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>( Socket().Query("serverConnection( target: $opc ){ name opcSessions{count} }", vars, true) );
		TRACE( "serverConnection: {}.", serialize(single) );
		Logout( sessionId );

		let connection = SelectServerCnnctn( OpcServerTarget ); ASSERT_TRUE( connection );
		bool found{};
		for( let& r : rows.as_array() ){
			let& o = r.as_object();
			EXPECT_TRUE( o.contains("target") ) << serialize( o ); //requested explicitly here, so it must not be erased.
			let count = Json::FindNumberPath<uint32>( o, "opcSessions/count" );
			ASSERT_TRUE( count ) << serialize( o );
			let connectionCount = Json::FindNumberPath<uint32>( o, "opcConnections/count" );
			ASSERT_TRUE( connectionCount ) << serialize( o );
			if( Json::AsNumber<ServerCnnctnPK>(o, "id")==connection->Id ){
				found = true;
				EXPECT_GE( *count, 1u );
				EXPECT_GE( *connectionCount, 1u ) << "a UAClient for the target is live - the serverDescription above connected it.";
			}
		}
		EXPECT_TRUE( found ) << serialize( rows );
		let& o = single.as_object();
		EXPECT_FALSE( o.contains("target") ) << serialize( o );
		EXPECT_GE( Json::FindNumberPath<uint32>(o, "opcSessions/count").value_or(0), 1u ) << serialize( o );
	}

	TEST_F( QLTests, serverConnectionIntrospection ){
		constexpr sv fieldsQL{ "{ fields{ name type{ name kind ofType{ name kind } } } }" };
		for( sv typeName : {"ServerConnection"sv, "serverConnections"sv} ){ //both spellings are declared in config/introspection/serverConnection.jsonnet.
			let value = Socket().QuerySync( Ƒ("__type( name: \"{}\" ){}", typeName, fieldsQL), {} );
			TRACE( "__type({}): {}.", typeName, serialize(value) );
			let& fields = Json::AsArray( value.as_object(), "fields" );
			auto find = [&]( sv name ){ return find_if( fields, [&](let& f){ return Json::AsSV(f.as_object(), "name")==name; } ); };
			ASSERT_NE( find("url"), fields.end() ) << serialize( value ); //extend:true keeps the DB columns.
			auto opcSessions = find( "opcSessions" );
			ASSERT_NE( opcSessions, fields.end() ) << serialize( value );
			EXPECT_EQ( Json::AsSVPath(opcSessions->as_object(), "type/kind"), "OBJECT" );
			EXPECT_EQ( Json::AsSVPath(opcSessions->as_object(), "type/name"), "OpcSessions" );
			auto opcConnections = find( "opcConnections" );
			ASSERT_NE( opcConnections, fields.end() ) << serialize( value );
			EXPECT_EQ( Json::AsSVPath(opcConnections->as_object(), "type/name"), "OpcConnections" );
		}
		for( sv typeName : {"OpcSessions"sv, "OpcConnections"sv} ){ //config-only types - no view behind them.
			let value = Socket().QuerySync( Ƒ("__type( name: \"{}\" ){}", typeName, fieldsQL), {} );
			TRACE( "__type({}): {}.", typeName, serialize(value) );
			let& fields = Json::AsArray( value.as_object(), "fields" );
			ASSERT_EQ( fields.size(), 1u ) << serialize( value );
			let& count = fields[0].as_object();
			EXPECT_EQ( Json::AsSV(count, "name"), "count" );
			EXPECT_EQ( Json::AsSVPath(count, "type/kind"), "NON_NULL" );
			EXPECT_EQ( Json::AsSVPath(count, "type/ofType/name"), "UInt" );
		}
	}

	TEST_F( QLTests, multipleQueries ){
		let q =
			"connection: serverConnection( target: $opc ){ id name target url certificateUri defaultBrowseNs }"
			"server: serverDescription( opc: $opc ){ applicationUri productUri applicationName applicationType gatewayServerUri discoveryProfileUri discoveryUrls }"
			"policy: securityPolicyUri( opc: $opc )"
			"mode: securityMode( opc: $opc )";
		const jobject vars{ {"opc", OpcServerTarget} };
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>(	Socket().Query(q, vars, false) );
		TRACE( "multipleQueries: {}.", serialize(value) );
		ASSERT_TRUE( serialize(value).size() );
	}
}