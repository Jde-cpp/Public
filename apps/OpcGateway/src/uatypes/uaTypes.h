#pragma once
#include <jde/fwk/str.h>
#include "ExpectedNodeId.h"

namespace Jde::Opc::Gateway{
	struct BrowsePathsToNodeIdResponse final : UA_TranslateBrowsePathsToNodeIdsResponse{
		BrowsePathsToNodeIdResponse( UA_TranslateBrowsePathsToNodeIdsResponse&& rhs, const vector<string>& paths, Handle uaHandle, SRCE )ε;
		~BrowsePathsToNodeIdResponse(){ UA_TranslateBrowsePathsToNodeIdsResponse_clear(this); }

		using Expected = ExpectedNodeId;
		operator flat_map<string,Expected>()Ε;
	private:
		α Path()Ι->string{ return _paths.empty() ? "" : _paths[0]; }
		const vector<string>& _paths;
		SL _sl;
	};
}