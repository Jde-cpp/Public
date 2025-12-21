#include "utils/GatewayClientSocket.h"
#include "utils/helpers.h"
#include <jde/web/client/proto/Web.FromServer.pb.h>
#include "../src/GatewayAppClient.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	struct QLTests : ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void{
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