﻿#pragma once
#include <jde/opc/uatypes/Node.h>
#include "Value.h"
#include <jde/framework/coroutine/Await.h>

namespace Jde::Opc{
	struct UAClient;
namespace Browse{
	α OnResponse( UA_Client *ua, void* userdata, RequestId requestId, UA_BrowseResponse* response )ι->void;

	struct FoldersAwait final : IAwait{
		FoldersAwait( NodeId&& node, sp<UAClient>& c, SRCE )ι:IAwait{sl}, _node{move(node)},_client{c}{}
		α Suspend()ι->void override;
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
		α ToJson( up<flat_map<NodeId, Value>>&& pSnapshot, flat_map<NodeId, NodeId>&& dataTypes )ε->jobject;
	};
	//ΓOPC α ObjectsFolder( sp<UAClient> ua, NodeId node, bool snapshot )ι->Task;

	struct ΓOPC ObjectsFolderAwait final : TAwait<jobject>{
		using base = TAwait<jobject>;
		ObjectsFolderAwait( NodeId node, bool snapshot, sp<UAClient> ua, SRCE )ι;
		α Suspend()ι->void override;
	private:
		α Execute()ι->Coroutine::Task;
		sp<UAClient> _ua; NodeId _node; bool _snapshot;
	};
}}