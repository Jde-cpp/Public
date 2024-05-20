#pragma once
#include <jde/iot/uatypes/Node.h>
#include "Value.h"
#include "../../../src/web/RestServer.h"

namespace Jde::Iot{
	struct UAClient;
namespace Browse{
	α Tag()ι->sp<LogTag>;
	ΓI α ObjectsFolder( sp<UAClient> ua, NodeId node, Web::Rest::Request req, bool snapShot )ι->Task;
	α OnResponse( UA_Client *ua, void* userdata, RequestId requestId, UA_BrowseResponse* response )ι->void;

	struct FoldersAwait final : IAwait
	{
		FoldersAwait( NodeId&& node, sp<UAClient>& c, SRCE )ι:IAwait{sl}, _node{move(node)},_client{c}{}
		α await_suspend( HCoroutine h )ι->void override;
		α await_resume()ι->AwaitResult override{ return _pPromise->MoveResult(); }
	private:
		NodeId _node;
		sp<UAClient> _client;
	};

	struct Request :UA_BrowseRequest{
		Request( NodeId&& node )ι;
		Request( Request&& x )ι:UA_BrowseRequest{ x }{ UA_BrowseRequest_init( &x );}
		Request( const Request& x )ι{ UA_BrowseRequest_copy( &x, this ); }
		~Request(){ UA_BrowseRequest_clear(this); }
	};

	struct Response : UA_BrowseResponse{
		Response( UA_BrowseResponse&& x )ι:UA_BrowseResponse{ x }{ UA_BrowseResponse_init( &x ); }
		Response( Response&& x )ι:UA_BrowseResponse{ x }{ UA_BrowseResponse_init( &x ); }
		~Response(){ UA_BrowseResponse_clear(this); }

		α Nodes()ι->flat_set<NodeId>;
		α ToJson( up<flat_map<NodeId, Value>>&& pSnapshot, flat_map<NodeId, NodeId>&& dataTypes )ε->json;
	};
}}