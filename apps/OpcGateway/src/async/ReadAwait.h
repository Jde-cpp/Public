#pragma once

namespace Jde::QL{ struct TableQL; }
namespace Jde::Opc::Gateway{
	namespace Browse{ struct Response; }
	struct UAClient;
	struct ReadRequest final : UA_ReadRequest{
		ReadRequest( const NodeId& nodeId, UA_AttributeId attrib )ι;
		ReadRequest( const NodeId& nodeId, const QL::TableQL& ql )ι;
		ReadRequest( Browse::Response&& browse, QL::TableQL&& ql )ι;
		α Add( const QL::TableQL& ql, const flat_map<NodeId, jobject>& nodes )ι->void;
		Ω AtribString( UA_AttributeId id )->const string&;
	private:
		vector<UA_ReadValueId> _readIds;
	};
	struct ReadResponse final : UA_ReadResponse{
		ReadResponse():UA_ReadResponse{}{}
		ReadResponse( ReadResponse&& rhs )ι;
		ReadResponse( UA_ReadResponse&& rhs )ι;
		~ReadResponse(){ UA_ReadResponse_clear(this); }
		α operator=( ReadResponse&& rhs )ι->ReadResponse&;
		α ScalerDataType()ι->UA_DataType*;
		α Validate( Handle uahandle, SL sl )ε->void;
		α GetJson()ι{ flat_map<NodeId, jobject> nodes; SetJson(nodes); return nodes; }
		α SetJson( flat_map<NodeId, jobject>& nodes )ι->void;
		optional<ReadRequest> Request;
	};

	struct ΓOPC ReadAwait final : TAwait<ReadResponse>{
		ReadAwait( ReadRequest&& req, sp<UAClient> c )ι:_request{move(req)}, _client{move(c)}{}
		ReadAwait( NodeId nodeId, UA_AttributeId attrib, sp<UAClient> c )ι:ReadAwait{{move(nodeId), attrib}, move(c)}{}
		ReadAwait( Browse::Response&& browse, QL::TableQL&& ql, sp<UAClient> c )ι:ReadAwait{{move(browse), move(ql)}, move(c)}{}

		α await_ready()ι->bool{ return !_request.nodesToReadSize; }
		α Suspend()ι->void override;
		Ω OnResponse( UA_Client* client, void* userdata, UA_UInt32 requestId, UA_ReadResponse* rr )ι->void;
		α await_resume()ι->ReadResponse;
	private:
		ReadRequest _request;
		RequestId 	_requestId{};
		sp<UAClient> _client;
	};
}