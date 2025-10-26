#include "uaTypes.h"
#include <jde/fwk/utils/collections.h>

#define let const auto
namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	BrowsePathsToNodeIdResponse::BrowsePathsToNodeIdResponse( UA_TranslateBrowsePathsToNodeIdsResponse&& rhs, const vector<string>& paths, Handle uaHandle, SL sl )ε:
		UA_TranslateBrowsePathsToNodeIdsResponse{ move(rhs) },
		_paths{ paths },
		_sl{ sl }{
		THROW_IFX( responseHeader.serviceResult, UAClientException(responseHeader.serviceResult, uaHandle, Ƒ("UA_Client_Service_translateBrowsePathsToNodeIds('{}').", Path()), sl) );
		THROW_IFX( resultsSize!=paths.size(), UAClientException(responseHeader.serviceResult, uaHandle, Ƒ("UA_Client_Service_translateBrowsePathsToNodeIds resultSize: {}, pathSize: {}", resultsSize, paths.size()), sl) );
	}

	BrowsePathsToNodeIdResponse::operator flat_map<string,BrowsePathsToNodeIdResponse::Expected>()Ε{
		auto y = ReserveMap<string,Expected>( resultsSize );
		for( uint i=0; i<resultsSize; ++i ){
			optional<Expected> expected;
			if( results[i].statusCode )
				expected = Expected{ std::unexpected{results[i].statusCode} };
			else if( results[i].targetsSize==0 )
				expected = Expected{ std::unexpected{UA_STATUSCODE_BADNOTFOUND} };
			else{
				if( results[i].targetsSize>1 )
					LOGSL( ELogLevel::Warning, _sl, _tags, "Multiple targets found for '{}'. Using first.", _paths[i] );
				expected = Expected{ ExNodeId{results[i].targets[0].targetId} };
			}
			y.emplace( _paths[i], move(*expected) );
		}
		return y;
	}
}