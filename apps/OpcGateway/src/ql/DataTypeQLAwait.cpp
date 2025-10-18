#include "DataTypeQLAwait.h"
#include "../async/ConnectAwait.h"
#include "../UAClient.h"

#define let const auto
namespace Jde::Opc::Gateway{
	α DataTypeQLAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		const NodeId nodeId{ _ql.Args };
		auto opcId{ Json::FindString(_ql.Args,"opc").value_or("") };
		auto client = co_await ConnectAwait{ move(opcId), move(_sessionInfo), _sl };

		UA_DataTypeArray *customTypes;
		if( auto sc = UA_Client_getRemoteDataTypes(*client, 1, &nodeId, &customTypes); sc )
			throw UAClientException{ sc, client->Handle(), Ƒ("Could not get data type: {}", nodeId.ToString()), _sl };
		if( customTypes->typesSize ){
			let dt = &customTypes->types[0];
			TRACET( ELogTags::Test, "DataType: {} for nodeId: {}", dt ? dt->typeName : "not found", nodeId.ToString() );
		}
		Resume( jvalue{} );
	}
}