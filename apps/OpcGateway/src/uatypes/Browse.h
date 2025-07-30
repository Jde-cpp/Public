#pragma once
#include <jde/opc/uatypes/Node.h>
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

		α Nodes()ι->flat_set<ExNodeId>;
		α ToJson( flat_map<ExNodeId, Value>&& snapshot, flat_map<ExNodeId, ExNodeId>&& dataTypes )ε->jobject;
	};

	struct FoldersAwait final : TAwait<Response>{
		FoldersAwait( ExNodeId node, sp<UAClient>& c, SRCE )ι:TAwait<Response>{sl}, _node{move(node)},_client{c}{}
		α Suspend()ι->void override;
//		α await_resume()ι->AwaitResult override{ return _pPromise->MoveResult(); }
	private:
		ExNodeId _node;
		sp<UAClient> _client;
	};

	struct Request :UA_BrowseRequest{
		Request( ExNodeId&& node )ι;
		Request( Request&& x )ι:UA_BrowseRequest{ x }{ UA_BrowseRequest_init( &x );}
		Request( const Request& x )ι{ UA_BrowseRequest_copy( &x, this ); }
		~Request(){ UA_BrowseRequest_clear(this); }
	};
}
	struct ΓOPC ObjectsFolderAwait final : TAwaitEx<jobject, TAwait<Browse::Response>::Task>{
		using base = TAwaitEx<jobject,TAwait<Browse::Response>::Task>;
		ObjectsFolderAwait( ExNodeId node, bool snapshot, sp<UAClient> ua, SRCE )ι;
	private:
		α Execute()ι->TAwait<Browse::Response>::Task;
		α Snapshot( Browse::Response response )ι->TAwait<flat_map<ExNodeId, Value>>::Task;
		α Attributes( Browse::Response response, flat_map<ExNodeId, Value> values={} )ι->TAwait<flat_map<ExNodeId, ExNodeId>>::Task;
		α Retry()ι->VoidAwait::Task;
		sp<UAClient> _ua; ExNodeId _node; bool _snapshot;
	};
}