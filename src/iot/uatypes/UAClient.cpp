#include <jde/iot/uatypes/UAClient.h>

#include <open62541/plugin/securitypolicy_default.h>
#include <jde/iot/async/CreateSubscriptions.h>
#include <jde/iot/uatypes/Node.h>
#include <jde/iot/uatypes/Value.h>
#include "Variant.h"
#include "../async/Attributes.h"
#include <jde/iot/async/DataChanges.h>
#include <jde/iot/async/SetMonitoringMode.h>
#include <jde/iot/async/Write.h>
#include "../uatypes/CreateMonitoredItemsRequest.h"

#define var const auto

namespace Jde::Iot{
	sp<LogTag> _logTag{ Logging::Tag("app.client") };
	α UAClient::LogTag()ι->sp<Jde::LogTag>{ return _logTag; }
	flat_map<tuple<string,string>,sp<UAClient>> _clients; shared_mutex _clientsMutex;
	concurrent_flat_set<sp<UAClient>> _awaitingActivation;

	UAClient::UAClient( str address, str userId, str password )ε:
		UAClient{ OpcServer{address}, userId, password }
	{}

	UAClient::UAClient( OpcServer&& opcServer, str userId, str password )ε:
		UserId{ userId },
		_opcServer{ move(opcServer) },
		_logger{ 0 },
		Password{ password },
		_ptr{ Create() },
		MonitoredNodes{ this }{
		UA_ClientConfig_setDefault( Configuration() );
		DBG( "[{:x}] Creating UAClient( '{}', '{}' )", Handle(), Target(), Url() );
//		DBG( "sizeof(UA_Client)={}", sizeof(UA_Client) );
	}

	α UAClient::Shutdown()ι->void{
		sl _1{ _clientsMutex };
		for( var& [_,p] : _clients ){
			p->MonitoredNodes.Shutdown();
			p->_asyncRequest.Stop();
			WARN_IF( p.use_count()>1, "[{:x}]use_count={}", p->Handle(), p.use_count() );
		}
		ul _{ _clientsMutex };
		_clients.clear();
	}
	α UAClient::Configuration()ε->UA_ClientConfig*{
		const fs::path root = IApplication::ApplicationDataFolder();
		const fs::path certificateFile = root/"cert.pem";
		const fs::path privateKeyFile = root/"private.pem";
		const string passcode = OSApp::EnvironmentVariable("JDE_PASSCODE");
		var uri = Str::Replace( _opcServer.CertificateUri, " ", "%20" );
		bool addSecurity = !uri.empty();//urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server
		//TODO - test no security also
		if( addSecurity && !fs::exists(certificateFile) ){
			if( !fs::exists(root) )
				fs::create_directories( root );
			if( !fs::exists(privateKeyFile) )
				Crypto::CreateKey( root/"public.pem", privateKeyFile, passcode );
			Crypto::CreateCertificate( certificateFile, privateKeyFile, passcode, Jde::format("URI:{}", uri), "jde-cpp", "US", "localhost" );
		}
		auto config = UA_Client_getConfig( _ptr ); 
		using SecurityPolicyPtr = up<UA_SecurityPolicy, decltype(&UA_free)>;
		const uint size = addSecurity ? 2 : 1; ASSERT( !config->securityPoliciesSize );
		SecurityPolicyPtr securityPolicies{ (UA_SecurityPolicy*)UA_malloc( sizeof(UA_SecurityPolicy)*size), &UA_free };
		auto sc = UA_SecurityPolicy_None(&securityPolicies.get()[0], UA_BYTESTRING_NULL, &_logger); THROW_IFX( sc, UAException(sc, _ptr, 0, ELogLevel::Debug) );
		if( addSecurity ){
			config->applicationUri = UA_STRING_ALLOC( uri.c_str() );
			config->clientDescription.applicationUri = UA_STRING_ALLOC( uri.c_str() );
			auto certificate = ToUAByteString( Crypto::ReadCertificate(certificateFile) );
			auto privateKey = ToUAByteString( Crypto::ReadPrivateKey(privateKeyFile, passcode) );
			sc = UA_SecurityPolicy_Basic256Sha256(&securityPolicies.get()[1], *certificate, *privateKey, &_logger ); THROW_IFX( sc, UAException(sc, _ptr, 0, ELogLevel::Debug) );
		}
		config->securityPolicies = securityPolicies.release();
		config->securityPoliciesSize = size;
		return config;
	}
	α UAClient::AddSessionAwait( HCoroutine&& h )ι->void{ 
		{
			lg _{_sessionAwaitableMutex}; 
			_sessionAwaitables.emplace_back( move(h) ); 
		}
		Process( std::numeric_limits<RequestId>::max(), nullptr );
	}
	α UAClient::TriggerSessionAwaitables()ι->void{
		vector<HCoroutine> handles;
		{
			lg _{_sessionAwaitableMutex}; 
			for_each(_sessionAwaitables, [&handles](auto&& h){handles.emplace_back(move(h));} );
			_sessionAwaitables.clear();
		}
		for_each( handles, [](auto&& h){h.resume();} );
	}

	α UAClient::StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void{
		constexpr std::array<sv,6> sessionStates = {"Closed", "CreateRequested", "Created", "ActivateRequested", "Activated", "Closing"};
		DBG( "[{:x}]channelState='{}', sessionState='{}', connectStatus='({:x}){}'", (uint)ua, UAException::Message(channelState), Str::FromEnum(sessionStates, sessionState), connectStatus, UAException::Message(connectStatus) );
		BREAK_IF( connectStatus );
		if( auto pClient = sessionState == UA_SESSIONSTATE_ACTIVATED ? UAClient::TryFind(ua) : sp<UAClient>{}; pClient ){
			pClient->TriggerSessionAwaitables();
			pClient->ClearRequest<UARequest>( std::numeric_limits<RequestId>::max() );
		}
	if( sessionState == UA_SESSIONSTATE_ACTIVATED || connectStatus==UA_STATUSCODE_BADIDENTITYTOKENINVALID || connectStatus==UA_STATUSCODE_BADCONNECTIONREJECTED || connectStatus==UA_STATUSCODE_BADINTERNALERROR || connectStatus==UA_STATUSCODE_BADUSERACCESSDENIED ){
			_awaitingActivation.erase_if( [ua, sessionState,connectStatus]( sp<UAClient> pClient){
				if( pClient->UAPointer()!=ua )return false;
				pClient->ClearRequest<UARequest>( std::numeric_limits<RequestId>::max() );//previous clear didn't have pClient
				var id{ make_tuple(pClient->Target(),pClient->UserId) };
				if( sessionState == UA_SESSIONSTATE_ACTIVATED ){
					{
						ul _{ _clientsMutex };
						ASSERT( _clients.find(id)==_clients.end() );
						_clients[id] = pClient;
					}
					ConnectAwait::Resume( move(pClient), get<0>(id), get<1>(id) );
				}
				else
					ConnectAwait::Resume( move(pClient), pClient->Target(), pClient->UserId, UAException{connectStatus} );
				return true;
			});
		}
	}
	α inactivityCallback(UA_Client *client)->void{
		BREAK;
	}
	α subscriptionInactivityCallback( UA_Client *client, SubscriptionId subscriptionId, void *subContext ){
		DBG( "[{:x}.{:x}]subscriptionInactivityCallback", (uint)client, subscriptionId );
	}
	α UAClient::Create()ι->UA_Client*{
		_config.logging = &_logger;
		_config.eventLoop = UA_EventLoop_new_POSIX( _config.logging );
		UA_ConnectionManager *tcpCM = UA_ConnectionManager_new_POSIX_TCP( "tcp connection manager"_uv );
		_config.eventLoop->registerEventSource(_config.eventLoop, (UA_EventSource*)tcpCM);
		_config.timeout = 10000; /*ms*/
		_config.stateCallback = StateCallback;
		_config.inactivityCallback = inactivityCallback;
		_config.subscriptionInactivityCallback = subscriptionInactivityCallback;
		if( UserId.size() )
			UA_ClientConfig_setAuthenticationUsername( &_config, UserId.c_str(), Password.c_str() );
		UA_ConnectionManager *udpCM = UA_ConnectionManager_new_POSIX_UDP( "udp connection manager"_uv );
		_config.eventLoop->registerEventSource( _config.eventLoop, (UA_EventSource*)udpCM );
		auto ua = UA_Client_newWithConfig( &_config );
		_logger.context = ua;
		UA_Client_getConfig(ua)->eventLoop->logger = _config.logging;

		return ua;
	}
	α UAClient::Connect()ε->void{
		auto p = shared_from_this();
		ASSERT( !_awaitingActivation.contains(p) );
		_awaitingActivation.emplace( shared_from_this() );
		var sc = UA_Client_connectAsync( UAPointer(), Url().c_str() ); THROW_IFX( sc, UAException(sc) );
		_asyncRequest.SetParent( p );
		Process( std::numeric_limits<RequestId>::max(), nullptr );
	}
	α UAClient::Process( RequestId requestId, up<UARequest>&& userData )ι->void{
		_asyncRequest.Process( requestId, move(userData) );
	}
	
	α UAClient::ProcessDataSubscriptions()ι->void{
		Process( 0, nullptr );
	}
	
	α UAClient::StopProcessDataSubscriptions()ι->void{
		ClearRequest<UARequest>( 0 );
	}

	α UAClient::Retry( function<void(sp<UAClient>&&, HCoroutine&&)> f, UAException e, sp<UAClient> pClient, HCoroutine h )ι->Task{
		//TODO limit retry attempts.
		var id{ make_tuple(pClient->Target(), pClient->UserId) }; var password = pClient->Password;
		{
			ul _{ _clientsMutex };
			if( auto p =_clients.find(id); p!=_clients.end() ){
				DBG( "[{:x}]Removing client: ({:x}){}.", pClient->Handle(), e.Code, e.what() );
				pClient->_asyncRequest.Stop();
				_clients.erase(p);
			}
			else
				DBG( "[{:x}]({:x}) - could not find client={:x}.", pClient->Handle(), e.Code, e.what() );
		}

		if( e.Code==UA_STATUSCODE_BADCONNECTIONCLOSED || e.Code==UA_STATUSCODE_BADSERVERNOTCONNECTED ){
			try{
				pClient = ( co_await GetClient(get<0>(id), get<1>(id), password) ).SP<UAClient>();
				f( move(pClient), move(h) );
			}
			catch( IException& e ){
				Resume( move(e), move(h) );
			}
		}
		else
			Resume( move(e), move(h) );
	}
	α UAClient::SendBrowseRequest( Browse::Request&& request, HCoroutine&& h )ι->void{
		try{
			RequestId requestId{};
			TRACET( Browse::Tag(), "[{:x}]SendBrowseRequest", Handle() );
			UAε( UA_Client_sendAsyncBrowseRequest(_ptr, &request, Browse::OnResponse, nullptr, &requestId) );
			Process( requestId, mu<UARequest>(move(h)) );
		}
		catch( UAException& e ){
			Retry( [r{move(request)}]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->SendBrowseRequest( move(r), move(h) );}, move(e), shared_from_this(), move(h) );
		}
	}

	α UAClient::SendReadRequest( const flat_set<NodeId>&& nodeIds, HCoroutine&& h )ι->void{
		if( nodeIds.empty() )
			return Resume( Exception{"no nodes sent"}, move(h) );
		flat_map<UA_UInt32, NodeId> ids;
		RequestId firstRequestId{};
		try{
			//assume 1st will fail if any, request all again if latter node failed.
			for( auto&& nodeId : nodeIds ){
				RequestId requestId{};
				UAε( UA_Client_readValueAttribute_async(_ptr, nodeId.nodeId, Read::OnResponse, (void*)(uint)firstRequestId, &requestId) );
				if( !firstRequestId )
					firstRequestId = requestId;
				ids.emplace( requestId, nodeId );
	 		}
			TRACET( Read::LogTag(), "[{:x}.{}]SendReadRequest - count={}", Handle(), firstRequestId, ids.size() );
			_readRequests.try_emplace( firstRequestId, UARequestMulti<Value>{move(ids)} );
			Process( firstRequestId, mu<UARequest>(move(h)) );
		}
		catch( UAException& e ){
			Retry( [n=move(nodeIds)]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->SendReadRequest( move(n), move(h) );}, move(e), shared_from_this(), move(h) );
		}
	}
	α UAClient::SendWriteRequest( flat_map<NodeId,Value>&& values, HCoroutine&& h )ι->void{
		if( values.empty() )
			return Resume( Exception("no nodes sent"), move(h) );
		flat_map<UA_UInt32, NodeId> ids;
		Iot::RequestId firstRequestId{};
		try{
			for( auto&& [nodeId, value] : values ){
				RequestId requestId{};
				UAε( UA_Client_writeValueAttribute_async(_ptr, nodeId.nodeId, &value.value, Write::OnResonse, (void*)(uint)firstRequestId, &requestId) );
				if( !firstRequestId )
					firstRequestId = requestId;
				ids.emplace( requestId, nodeId );
	 		}
			_writeRequests.try_emplace( firstRequestId, UARequestMulti<UA_WriteResponse>{move(ids), shared_from_this()} );
			Process( firstRequestId, mu<UARequest>(move(h)) );
		}
		catch( UAException& e ){
			Retry( [n=move(values)]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->SendWriteRequest( move(n), move(h) );}, move(e), shared_from_this(), move(h) );
		}
	}
	α UAClient::SetMonitoringMode( Iot::SubscriptionId subscriptionId )ι->void{
		UA_SetMonitoringModeRequest request{};
		request.subscriptionId = subscriptionId;
		try{
			RequestId requestId{};
			UAε( UA_Client_MonitoredItems_setMonitoringMode_async(UAPointer(), move(request), SetMonitoringModeCallback, nullptr, &requestId) );
			Process( requestId, nullptr );//if retry fails, don't want to process.
		}
		catch( UAException& e ){
			Retry( [subscriptionId]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->SetMonitoringMode( subscriptionId );}, move(e), shared_from_this(), {} );
		}
	}

	α UAClient::CreateSubscriptions()ι->void{
		try{
			RequestId requestId{};
			UAε( UA_Client_Subscriptions_create_async(UAPointer(), UA_CreateSubscriptionRequest_default(), nullptr, StatusChangeNotificationCallback, DeleteSubscriptionCallback, CreateSubscriptionCallback, nullptr, &requestId) );
			TRACET( UAMonitoringNodes::LogTag(), "[{:x}.{:x}]CreateSubscription", Handle(), requestId );
			Process( requestId, nullptr );
		}
		catch( UAException& e ){
			Retry( []( sp<UAClient>&& p, HCoroutine&& h )mutable{p->CreateSubscriptions();}, move(e), shared_from_this(), {} );
		}
	}

	α UAClient::DataSubscriptions( CreateMonitoredItemsRequest&& request, Jde::Handle requestHandle, HCoroutine&& h )ι->void{
		ASSERT(CreatedSubscriptionResponse);
		if( !CreatedSubscriptionResponse )
			return Resume( Exception{"CreatedSubscriptionResponse==null"}, move(h) );
		try{
			vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbacks{request.itemsToCreateSize, DataChangesDeleteCallback};
			vector<UA_Client_DataChangeNotificationCallback> dataChangeCallbacks{request.itemsToCreateSize, DataChangesCallback};
			void** contexts = nullptr;
			request.subscriptionId = CreatedSubscriptionResponse->subscriptionId;

			RequestId requestId{};
			UAε( UA_Client_MonitoredItems_createDataChanges_async(UAPointer(), request, contexts, dataChangeCallbacks.data(), deleteCallbacks.data(), CreateDataChangesCallback, (void*)requestHandle, &requestId) );
			TRACET( UAMonitoringNodes::LogTag(), "[{:x}.{:x}]DataSubscriptions - {}", Handle(), requestId, request.ToJson().dump() );
			Process( requestId, mu<UARequest>(move(h)) );//TODO handle BadSubscriptionIdInvalid
		}
		catch( UAException& e ){
			Resume( move(e), move(h) );//retry will leave CreatedSubscriptionResponse null...
			//Retry( [&x=request,requestHandle]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->DataSubscriptions(move(x), requestHandle, move(h));}, move(e), shared_from_this(), move(h) );
		}
	}

	α UAClient::DataSubscriptionDelete( Iot::SubscriptionId subscriptionId, flat_set<MonitorId>&& monitoredItemIds )ι->void{
		UA_DeleteMonitoredItemsRequest request; UA_DeleteMonitoredItemsRequest_init(&request);
		request.subscriptionId = subscriptionId;
		request.monitoredItemIdsSize = monitoredItemIds.size();
		request.monitoredItemIds = (UA_UInt32*)UA_Array_new( monitoredItemIds.size(), &UA_TYPES[UA_TYPES_UINT32] );
		uint i=0;
		for( auto& id : monitoredItemIds )
			request.monitoredItemIds[i++] = id;
		try{
			RequestId requestId{};
			UAε( UA_Client_MonitoredItems_delete_async(UAPointer(), move(request), MonitoredItemsDeleteCallback, nullptr, &requestId) );
			Process( requestId, nullptr );
			UA_DeleteMonitoredItemsRequest_clear( &request );
		}
		catch( const IException& )
		{}
	}

	α UAClient::RequestDataTypeAttributes( const flat_set<NodeId>&& x, HCoroutine&& h )ι->void
	{
		flat_map<UA_UInt32, NodeId> ids;
		Iot::RequestId firstRequestId{};
		try{
			for( var& node : x ){
				RequestId requestId{};
				UAε( UA_Client_readDataTypeAttribute_async(_ptr, node.nodeId, Attributes::OnResonse, (void*)(uint)firstRequestId, &requestId) );
				if( !firstRequestId )
					firstRequestId = requestId;
				ids.emplace( requestId, node );
	 		}
			_dataAttributeRequests.try_emplace( firstRequestId, UARequestMulti<NodeId>{move(ids), shared_from_this()} );
			Process( firstRequestId, mu<UARequest>(move(h)) );
		}
		catch( UAException& e ){
			Retry( [n=move(x)]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->RequestDataTypeAttributes( move(n), move(h) );}, move(e), shared_from_this(), move(h) );
		}
	}

	α UAClient::Unsubscribe( const sp<IDataChange>&& dataChange )ι->void{
		sl _{ _clientsMutex };
		for( auto& [_, pClient] : _clients )
			pClient->MonitoredNodes.Unsubscribe( dataChange );
	}

	α UAClient::Find( UA_Client* ua, SL srce )ε->sp<UAClient>{
		sp<UAClient> y = TryFind( ua, srce );
		if( !y )
	 		throw Exception{ srce, ELogLevel::Debug, "[{}]Could not find client.", format("{:x}", (uint)ua) };
		return y;
	}

	α UAClient::TryFind( UA_Client* ua, SL srce )ι->sp<UAClient>{
		sl _{ _clientsMutex };
		sp<UAClient> y;
		if( IApplication::ShuttingDown() )
			Logging::Log( Logging::Message{ELogLevel::Trace, "Application is shutting down.", srce}, _logTag );
		else if( auto p = find_if(_clients, [ua](var& c){return c.second->_ptr==ua;}); p!=_clients.end() )
			y = p->second;
		return y;
	}

	α UAClient::Find( str id, str userId )ι->sp<UAClient>{
		sl _{ _clientsMutex };
		sp<UAClient> y;
		if( auto p = id.size() ? _clients.find(make_tuple(id,userId)) : find_if( _clients, [&userId](var& c){return c.second->IsDefault() && c.second->UserId==userId;} ); p!=_clients.end() )
			y = p->second;
		return y;
	}

	UAClient::~UAClient() {
#undef free
		_config.eventLoop->free(_config.eventLoop);
#define free _free_dbg
		UA_Client_delete(_ptr);
		DBG("[{:x}]~UAClient( '{}', '{}' )", Handle(), Target(), Url());
	}
}