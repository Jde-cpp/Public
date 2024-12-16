#pragma once
#include "../exports.h"
#include "Browse.h"
#include "Logger.h"
#include "../async/AsyncRequest.h"
#include "../async/ConnectAwait.h"
#include "../types/OpcServer.h"
#include "../types/MonitoringNodes.h"

namespace Jde::Opc{
	namespace Read{ α OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *let )ι->void; }
	namespace Write{ α OnResonse( UA_Client *ua, void *userdata, RequestId requestId, UA_WriteResponse *response )ι->void; }
	namespace Attributes{α OnResonse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void;}

	struct CreateMonitoredItemsRequest;
	struct Value;

	struct ΓOPC UAClient final : std::enable_shared_from_this<UAClient>{
		UAClient( OpcServer&& opcServer, str userId, str password )ε;
		UAClient( str address, str userId, str password )ε;
		~UAClient();

		operator UA_Client* ()ι{ return _ptr; }
		Ω Shutdown( bool terminate )ι->void;
		Ω GetClient( string id, string userId, string pw, SRCE )ι{ return ConnectAwait{move(id), move(userId), move(pw), sl}; }
		Ω Find( str id, str userId, str pw )ι->sp<UAClient>;
		Ω Find( UA_Client* ua, SRCE )ε->sp<UAClient>;
		Ω TryFind( UA_Client* ua, SRCE )ι->sp<UAClient>;
		α SubscriptionId()Ι->SubscriptionId{ return CreatedSubscriptionResponse ? CreatedSubscriptionResponse->subscriptionId : 0;}
		α CreateSubscriptions()ι->void;
		α DataSubscriptions( CreateMonitoredItemsRequest&& r, Handle requestHandle, HCoroutine&& h )ι->void;
		α DataSubscriptionDelete( Opc::SubscriptionId subscriptionId, flat_set<MonitorId>&& monitoredItemIds )ι->void;

		α SendBrowseRequest( Browse::Request&& request, HCoroutine h )ι->void;
		α SendReadRequest( const flat_set<NodeId>&& nodes, HCoroutine h )ι->void;
		α SendWriteRequest( flat_map<NodeId,Value>&& values, HCoroutine h )ι->void;
		α SetMonitoringMode( Opc::SubscriptionId subscriptionId )ι->void;
		α RequestDataTypeAttributes( const flat_set<NodeId>&& x, HCoroutine h )ι->void;
		Ṫ ClearRequest( UA_Client* ua, RequestId requestId )ι->up<T>;
		Ω ClearRequestH( UA_Client* ua , RequestId requestId)ι->HCoroutine{ auto r = ClearRequest<UARequest>( ua, requestId ); return r ? r->CoHandle : nullptr; }
		Ŧ ClearRequest( RequestId requestId )ι->up<T>;
		α ClearRequestH( RequestId requestId )ι->HCoroutine{ return ClearRequest<UARequest>( requestId )->CoHandle; }
		Ω Retry( function<void(sp<UAClient>&&, HCoroutine&&)> f, UAException e, sp<UAClient> pClient, HCoroutine h )ι->ConnectAwait::Task;
		α Process( RequestId requestId, up<UARequest>&& userData )ι->void;
		α ProcessDataSubscriptions()ι->void;
		α StopProcessDataSubscriptions()ι->void;
		α AddSessionAwait( HCoroutine h )ι->void;
		α TriggerSessionAwaitables()ι->void;

		α Target()ι->str{ return _opcServer.Target; }
		α Url()ι->str{ return _opcServer.Url; }
		α IsDefault()ι->bool{ return _opcServer.IsDefault; }
		α Handle()ι->uint{ return (uint)_ptr;}
		α UAPointer()ι->UA_Client*{return _ptr;}
		sp<UA_SetMonitoringModeResponse> MonitoringModeResponse;
		sp<UA_CreateSubscriptionResponse> CreatedSubscriptionResponse;
		UA_ClientConfig _config{};//TODO move private.
		string UserId;
	private:
		Ω LogTag()ι->sp<Jde::LogTag>;
		α Configuration()ε->UA_ClientConfig*;
		α Create()ι->UA_Client*;
		α Connect()ε->void;
		OpcServer _opcServer;

		concurrent_flat_map<Jde::Handle, UARequestMulti<Value>> _readRequests;
		concurrent_flat_map<Jde::Handle, UARequestMulti<UA_WriteResponse>> _writeRequests;
		concurrent_flat_map<Jde::Handle, UARequestMulti<NodeId>> _dataAttributeRequests;
		vector<HCoroutine> _sessionAwaitables; mutable mutex _sessionAwaitableMutex;

		AsyncRequest _asyncRequest;
		Logger _logger;
		string Password;
		UA_Client* _ptr{};//needs to be after _logger, _config, Password.
		friend ConnectAwait;
		friend α Read::OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *let )ι->void;
		friend α Write::OnResonse( UA_Client *ua, void *userdata, RequestId requestId, UA_WriteResponse *response )ι->void;
		friend α Attributes::OnResonse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void;
	public:
		Ω Unsubscribe( const sp<IDataChange>&& dataChange )ι->void;
		Ω StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void;
		UAMonitoringNodes MonitoredNodes;//destroy first
	};

#define _logTag LogTag()
	Ŧ UAClient::ClearRequest( UA_Client* ua, RequestId requestId )ι->up<T>{
		auto p = TryFind( ua );
		return p ? p->ClearRequest<T>( requestId ) : up<T>{};
	}

	Ŧ UAClient::ClearRequest( RequestId requestId )ι->up<T>{
		return _asyncRequest.ClearRequest<T>( requestId );
	}
}
#undef _logTag