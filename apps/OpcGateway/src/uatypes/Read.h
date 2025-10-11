#pragma once

namespace Jde::QL{ struct TableQL; }
namespace Jde::Opc::Gateway{
	namespace Browse{ struct Response; }
	struct ReadRequest final : UA_ReadRequest{
		ReadRequest( const NodeId& nodeId, const QL::TableQL& ql )ι;
		ReadRequest( const Browse::Response& browse, const QL::TableQL& ql )ι;
		Ω AtribString( UA_AttributeId id )->const string&;
	private:
		vector<UA_ReadValueId> _readIds;
	};
	struct ReadResponse final : UA_ReadResponse{
		ReadResponse( UA_ReadResponse&& rhs, ReadRequest&& request, Handle uaHandle, SRCE )ε;
		~ReadResponse(){ UA_ReadResponse_clear(this); }
		α GetJson()ι{ flat_map<NodeId, jobject> nodes; SetJson(nodes); return nodes; }
		α SetJson( flat_map<NodeId, jobject>& nodes )ι->void;
		ReadRequest Request;
	};
}