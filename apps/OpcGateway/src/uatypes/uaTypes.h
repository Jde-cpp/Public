#pragma once
#include <jde/fwk/str.h>

namespace Jde::QL{ struct TableQL; }
namespace Jde::Opc::Gateway{
	struct BrowsePathsToNodeIdResponse final : UA_TranslateBrowsePathsToNodeIdsResponse{
		BrowsePathsToNodeIdResponse( UA_TranslateBrowsePathsToNodeIdsResponse&& rhs, const vector<string>& paths, Handle uaHandle, SRCE )ε;
		~BrowsePathsToNodeIdResponse(){ UA_TranslateBrowsePathsToNodeIdsResponse_clear(this); }

		using Expected = std::expected<ExNodeId,StatusCode>;
		operator flat_map<string,Expected>()Ε;
	private:
		α Path()Ι->string{ return _paths.empty() ? "" : _paths[0]; }
		const vector<string>& _paths;
		SL _sl;
	};

	struct ReadRequest final : UA_ReadRequest{
		ReadRequest( const QL::TableQL& ql, const string& reqPath, const flat_map<string, std::expected<ExNodeId,StatusCode>>& pathNodes, jobject& jReqNode, flat_map<NodeId, jobject>& jparents )ι;
//		ReadRequest( ReadRequest&& x )ι:UA_ReadRequest{ x }{ UA_ReadRequest_init( &x ); }
//		ReadRequest( const ReadRequest& x )ι{ UA_ReadRequest_copy( &x, this ); }
		~ReadRequest(){}//_readIds holds the memory
		//vector<std::pair<ExNodeId,UA_AttributeId>> _readMembers;
		Ω AtribString( UA_AttributeId id )->const string&;
	private:
		vector<UA_ReadValueId> _readIds;
	};
	struct ReadResponse final : UA_ReadResponse{
		ReadResponse( UA_ReadResponse&& rhs, Handle uaHandle, SRCE )ε;
		~ReadResponse(){ UA_ReadResponse_clear(this); }
	};

}