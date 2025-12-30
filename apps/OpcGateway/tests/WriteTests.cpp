#include "utils/GatewayClientSocket.h"
#include "utils/helpers.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	struct WriteTests : ::testing::Test{

	};

	TEST_F( WriteTests, Enum ){
		jobject vars{ {"opc", OpcServerTarget}, {"id", jobject{{"ns", 4}, {"i", 6001}}} };
		let value = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>(	Socket().Query("variable( opc: $opc, id: $id ){ value }", vars, true) );
		TRACET( ELogTags::Test, "Read enum value: {}.", serialize(value) );
		vars["value"] = 2;
		let afterWrite = BlockAwait<Web::Client::ClientSocketAwait<jvalue>,jvalue>(	Socket().Query("updateVariable( opc: $opc, id: $id, value: $value ){ value }", vars, true) );
		TRACET( ELogTags::Test, "After write enum value: {}.", serialize(afterWrite) );
		ASSERT_EQ( afterWrite.as_object().at("value").as_int64(), 2 );
	}
}