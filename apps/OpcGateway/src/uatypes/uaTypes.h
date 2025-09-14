#pragma once
#include <jde/framework/str.h>

namespace Jde::Opc::Gateway{
	struct GetNodeIdResponse final : UA_TranslateBrowsePathsToNodeIdsResponse{
		GetNodeIdResponse( UA_TranslateBrowsePathsToNodeIdsResponse&& rhs, const vector<sv>& segments, Handle uaHandle, SRCE )ε;
		~GetNodeIdResponse(){ UA_TranslateBrowsePathsToNodeIdsResponse_clear(this); }

		operator ExNodeId()Ε;
	private:
		α Path()Ι->string{ return Str::Join(_segments, "/"); }
		const vector<sv>& _segments;
		SL _sl;
	};
	struct ReadResponse final : UA_ReadResponse{
		ReadResponse( UA_ReadResponse&& rhs, Handle uaHandle, NodeId id, SRCE )ε;
		~ReadResponse(){ UA_ReadResponse_clear(this); }
	};

}