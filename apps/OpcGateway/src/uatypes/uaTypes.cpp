#include "uaTypes.h"

namespace Jde::Opc::Gateway{

	GetNodeIdResponse::GetNodeIdResponse( UA_TranslateBrowsePathsToNodeIdsResponse&& rhs, const vector<sv>& segments, Handle uaHandle, SL sl )ε:
		UA_TranslateBrowsePathsToNodeIdsResponse{ move(rhs) },
		_segments{ segments },
		_sl{ sl }{
		THROW_IFX( results->statusCode, UAClientException(results->statusCode, Ƒ("UA_Client_Service_translateBrowsePathsToNodeIds('{}').", Path()), uaHandle, sl) );
	}

	GetNodeIdResponse::operator Jde::Opc::ExNodeId()Ε{
		THROW_IF( resultsSize!=1, "path: {}, resultsSize:'{}'.", Path(), resultsSize );
		THROW_IF( results[0].targetsSize!=1, "path:{}, targetsSize='{}'.", Path(), results[0].targetsSize );
		return ExNodeId{ move(results[0].targets[0].targetId) };
	}

	ReadResponse::ReadResponse( UA_ReadResponse&& rhs, Handle uaHandle, NodeId id, SL sl )ε:
		UA_ReadResponse{ move(rhs) }{
		THROW_IFX( rhs.responseHeader.serviceResult, UAClientException(rhs.responseHeader.serviceResult, Ƒ("UA_Client_Service_read('{}').", id.ToString()), uaHandle, sl) );
	}
}