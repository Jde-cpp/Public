#pragma once
//#include "../exports.h"
#include <jde/opc/uatypes/Logger.h>
#include "async/AsyncRequest.h"
#include "async/Attributes.h"
#include "async/ConnectAwait.h"
#include "async/DataChanges.h"
#include "async/ReadAwait.h"
#include "async/Write.h"
#include "auth/OpcServerSession.h"
#include "types/OpcClient.h"
#include "types/MonitoringNodes.h"
#include "uatypes/Browse.h"

namespace Jde::Opc{	struct Value; }
namespace Jde::Opc::Gateway{
	//struct Credential;
	namespace Browse{ struct Request; }
	namespace Read{ α OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *value )ι->void; }
	namespace Write{ α OnResponse( UA_Client *ua, void *userdata, RequestId requestId, UA_WriteResponse *response )ι->void; }
	namespace Attributes{α OnResponse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void;}

	struct CreateMonitoredItemsRequest;

	struct UAClient final : std::enable_shared_from_this<UAClient>{
		UAClient( OpcClient&& opcServer, Credential cred )ε;
		UAClient( str address, Credential cred )ε;
		~UAClient();

		operator UA_Client* ()ι{ return _ptr; }
		Ω Shutdown( bool terminate )ι->void;
		Ω GetClient( string id, Credential cred, SRCE )ι{ return ConnectAwait{move(id), move(cred), sl}; }
		Ω Find( str id, optional<Credential> cred )ι->sp<UAClient>;
		Ω Find( UA_Client* ua, SRCE )ε->sp<UAClient>;
		Ω TryFind( UA_Client* ua, SRCE )ι->sp<UAClient>;
		α SubscriptionId()Ι->SubscriptionId{ return CreatedSubscriptionResponse ? CreatedSubscriptionResponse->subscriptionId : 0;}
		α CreateSubscriptions()ι->void;
		α DataSubscriptions( CreateMonitoredItemsRequest&& r, Handle requestHandle, DataChangeAwait::Handle h )ι->void;
		α DataSubscriptionDelete( Gateway::SubscriptionId subscriptionId, flat_set<MonitorId>&& monitoredItemIds )ι->void;

		α SendBrowseRequest( Browse::Request&& request, Browse::FoldersAwait::Handle h )ι->void;
		α SendReadRequest( const flat_set<ExNodeId>&& nodes, ReadAwait::Handle h )ι->void;
		α SendWriteRequest( flat_map<ExNodeId,Value>&& values, WriteAwait::Handle h )ι->void;
		α SetMonitoringMode( Gateway::SubscriptionId subscriptionId )ι->void;
		α RequestDataTypeAttributes( const flat_set<ExNodeId>&& x, AttribAwait::Handle h )ι->void;
		Ω ClearRequest( UA_Client* ua, RequestId requestId )ι->void;
		Ṫ ClearRequestH( UA_Client* ua , RequestId requestId )ι->T;/*{
			auto r = ClearRequest<T>( ua, requestId );
			return r ? r->CoHandle : nullptr;
		}*/
		α ClearRequest( RequestId requestId )ι->void{ _asyncRequest.Clear( requestId ); }
		Ŧ ClearRequestH( RequestId requestId )ι->T;//{ return ClearRequest<UARequest<T>>( requestId )->CoHandle; }
		Ŧ Retry( function<void(sp<UAClient>&&, T)> f, UAException e, sp<UAClient> pClient, T h )ι->ConnectAwait::Task;
		α RetryVoid( function<void(sp<UAClient>&&) > f, UAException e, sp<UAClient> pClient )ι->ConnectAwait::Task;
		α Process( RequestId requestId, coroutine_handle<>&& h )ι->void;
		α Process( RequestId requestId )ι->void;
		α ProcessDataSubscriptions()ι->void;
		α StopProcessDataSubscriptions()ι->void;
		α AddSessionAwait( VoidAwait<>::Handle h )ι->void;
		α TriggerSessionAwaitables()ι->void;

		α Target()ι->const OpcClientNK&{ return _opcServer.Target; }
		α Url()ι->str{ return _opcServer.Url; }
		α IsDefault()ι->bool{ return _opcServer.IsDefault; }
		α Handle()ι->uint{ return (uint)_ptr;}
		α UAPointer()ι->UA_Client*{return _ptr;}
		sp<UA_SetMonitoringModeResponse> MonitoringModeResponse;
		sp<UA_CreateSubscriptionResponse> CreatedSubscriptionResponse;
		UA_ClientConfig _config{};//TODO move private.
		Gateway::Credential Credential;
	private:
		α Configuration()ε->UA_ClientConfig*;
		α Create()ι->UA_Client*;
		α Connect()ε->void;
		Ω RemoveClient( sp<UAClient>& client )ι->bool;

		OpcClient _opcServer;

		concurrent_flat_map<Jde::Handle, UARequestMulti<Value>> _readRequests;
		concurrent_flat_map<Jde::Handle, UARequestMulti<UA_WriteResponse>> _writeRequests;
		concurrent_flat_map<Jde::Handle, UARequestMulti<ExNodeId>> _dataAttributeRequests;
		vector<VoidAwait<>::Handle> _sessionAwaitables; mutable mutex _sessionAwaitableMutex;

		AsyncRequest _asyncRequest;
		Logger _logger;
		UA_Client* _ptr{};//needs to be after _logger, _config, Password.
		friend ConnectAwait;
		friend α Read::OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *value )ι->void;
		friend α Write::OnResponse( UA_Client *ua, void *userdata, RequestId requestId, UA_WriteResponse *response )ι->void;
		friend α Attributes::OnResponse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void;
	public:
		Ω Unsubscribe( const sp<IDataChange>&& dataChange )ι->void;
		Ω StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void;
		UAMonitoringNodes MonitoredNodes;//destroy first
	};

	Ŧ UAClient::ClearRequestH( UA_Client* ua, RequestId requestId )ι->T{
		auto p = TryFind( ua );
		return p ? p->ClearRequestH<T>( requestId ) : T{};
	}

	Ŧ UAClient::ClearRequestH( RequestId requestId )ι->T{
		return _asyncRequest.ClearHandle<T>( requestId );
	}

#define let const auto
	Ŧ UAClient::Retry( function<void(sp<UAClient>&&, T)> f, UAException e, sp<UAClient> client, T h )ι->ConnectAwait::Task{
		//TODO limit retry attempts.
		RemoveClient( client );
		if( e.Code==UA_STATUSCODE_BADCONNECTIONCLOSED || e.Code==UA_STATUSCODE_BADSERVERNOTCONNECTED ){
			try{
				client = co_await GetClient( client->Target(), client->Credential );
				f( move(client), h );
			}
			catch( exception& e ){
				h.promise().ResumeExp( move(e), h );
			}
		}
		else
			h.promise().ResumeExp( move(e), h );
	}
}
#undef let