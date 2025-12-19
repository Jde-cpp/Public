#include "utils/GatewayClientSocket.h"
#include "utils/helpers.h"

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
}