#pragma once
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>
//#include <jde/framework/coroutine/Await.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
namespace Browse{
	α OnResponse( UA_Client *ua, void* userdata, RequestId requestId, UA_BrowseResponse* response )ι->void;

	struct Response : UA_BrowseResponse{
		Response()ι{ ASSERT( false ); }
		Response( UA_BrowseResponse&& x )ι:UA_BrowseResponse{ x }{ UA_BrowseResponse_init( &x ); }
		Response( Response&& x )ι:UA_BrowseResponse{ x }{ UA_BrowseResponse_init( &x ); }
		~Response(){ UA_BrowseResponse_clear(this); }
		α operator=( Response&& x )ι->Response&;

		α Nodes()ι->flat_set<NodeId>;
		α Variables()ι->flat_set<NodeId>;
		α ToJson( flat_map<NodeId, Value>&& snapshot, flat_map<NodeId, NodeId>&& dataTypes )ε->jobject;
	};

	struct FoldersAwait final : TAwait<Response>{
		FoldersAwait( NodeId node, sp<UAClient>& c, SRCE )ι:TAwait<Response>{sl}, _node{move(node)},_client{c}{}
		α Suspend()ι->void override;
//		α await_resume()ι->AwaitResult override{ return _pPromise->MoveResult(); }
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
}
	struct ΓOPC ObjectsFolderAwait final : TAwaitEx<jobject, TAwait<Browse::Response>::Task>{
		using base = TAwaitEx<jobject,TAwait<Browse::Response>::Task>;
		ObjectsFolderAwait( NodeId node, bool snapshot, sp<UAClient> ua, SRCE )ι;
	private:
		α Execute()ι->TAwait<Browse::Response>::Task;
		α Snapshot( Browse::Response response )ι->TAwait<flat_map<NodeId, Value>>::Task;
		α Attributes( flat_set<NodeId>&& variables, Browse::Response response, flat_map<NodeId, Value> values={} )ι->TAwait<flat_map<NodeId, NodeId>>::Task;
		α Retry()ι->VoidAwait::Task;
		sp<UAClient> _ua; NodeId _node; bool _snapshot;
	};
}