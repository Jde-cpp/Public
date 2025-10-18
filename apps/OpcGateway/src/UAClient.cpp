#include "UAClient.h"

#include <open62541/plugin/securitypolicy_default.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/collections.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>
#include "StartupAwait.h"
#include "async/Attributes.h"
#include "async/CreateSubscriptions.h"
#include "async/DataChanges.h"
#include "async/SetMonitoringMode.h"
#include "async/Write.h"
#include "uatypes/Browse.h"
#include "uatypes/CreateMonitoredItemsRequest.h"
#include "uatypes/uaTypes.h"

#define let const auto

namespace Jde::Opc::Gateway{
//	using Client::UAClientException;
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	flat_map<ServerCnnctnNK,flat_map<Credential,sp<UAClient>>> _clients; shared_mutex _clientsMutex;
	α UAClient::RemoveClient( sp<UAClient>&& client )ι->bool{
		client->Connected = false;
		bool erased{};
		ul _{ _clientsMutex };
		if( auto serverCreds = _clients.find(client->Target()); serverCreds!=_clients.end() ){
			if( auto cred = serverCreds->second.find(client->Credential); cred!=serverCreds->second.end() ){
				DBG( "[{:x}]Removing client: '{}'.", client->Handle(), client->Target() );
				serverCreds->second.erase( cred );
				erased = true;
				if( serverCreds->second.empty() )
					_clients.erase( serverCreds );

			}
		}
		client = nullptr;
		if( !erased )
			DBG( "[{:x}] - could not find client='{}'.", client->Handle(), client->Target() );
		return erased;
	}
	concurrent_flat_set<sp<UAClient>> _awaitingActivation;

	UAClient::UAClient( str address, Gateway::Credential cred )ε:
		UAClient{ ServerCnnctn{address}, move(cred) }
	{}

	UAClient::UAClient( ServerCnnctn&& opcServer, Gateway::Credential cred )ε:
		Credential{ move(cred) },
		_opcServer{ move(opcServer) },
		_logger{ 0 },
		_ptr{ Create() },
		MonitoredNodes{ this }{
		UA_ClientConfig_setDefault( Configuration() );
		DBG( "[{:x}]Creating UAClient target: '{}' url: '{}' credential: '{}' )", Handle(), Target(), Url(), Credential.ToString() );
	}

	α UAClient::Shutdown( bool /*terminate*/ )ι->void{
		{
			sl _1{ _clientsMutex };
			for( auto& [_,creds] : _clients ){
				for( auto& [_,client] : creds ){
					client->MonitoredNodes.Shutdown();
					client->_asyncRequest.Stop();
					if( client.use_count()>1 )
					  WARN( "[{:x}]use_count={}", client->Handle(), client.use_count() );
				}
			}
		}
		ul _{ _clientsMutex };
		_clients.clear();
	}
	α UAClient::Configuration()ε->UA_ClientConfig*{
		const fs::path root = RootSslDir();
		const fs::path privateKeyFile = PrivateKeyFile();
		const string passcode = Passcode();
		let uri = Str::Replace( _opcServer.CertificateUri, " ", "%20" );
		bool addSecurity = !uri.empty();//urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server
		//TODO - test no security also
		if( addSecurity && !fs::exists(CertificateFile()) ){
			if( !fs::exists(root) )
				fs::create_directories( root );
			if( !fs::exists(privateKeyFile) )
				Crypto::CreateKey( root/Ƒ("public/{}.pem", Target()), privateKeyFile, passcode );
			Crypto::CreateCertificate( CertificateFile(), privateKeyFile, passcode, Jde::format("URI:{}", uri), "jde-cpp", "US", "localhost" );
		}
		auto config = UA_Client_getConfig( _ptr );
		using SecurityPolicyPtr = up<UA_SecurityPolicy, decltype(&UA_free)>;
		const uint size = addSecurity ? 2 : 1; ASSERT( !config->securityPoliciesSize );
		SecurityPolicyPtr securityPolicies{ (UA_SecurityPolicy*)UA_malloc( sizeof(UA_SecurityPolicy)*size), &UA_free };
		auto sc = UA_SecurityPolicy_None(&securityPolicies.get()[0], UA_BYTESTRING_NULL, &_logger); THROW_IFX( sc, UAClientException(sc, Handle()) );
		if( addSecurity ){
			config->applicationUri = UA_STRING_ALLOC( uri.c_str() );
			config->clientDescription.applicationUri = UA_STRING_ALLOC( uri.c_str() );
			auto certificate = ToUAByteString( Crypto::ReadCertificate(CertificateFile()) );
			auto privateKey = ToUAByteString( Crypto::ReadPrivateKey(privateKeyFile, passcode) );
			sc = UA_SecurityPolicy_Basic256Sha256( &securityPolicies.get()[1], *certificate, *privateKey, &_logger ); THROW_IFX( sc, UAClientException(sc, Handle()) );

			config->authSecurityPolicies = (UA_SecurityPolicy *)UA_realloc( config->authSecurityPolicies, sizeof(UA_SecurityPolicy) *(config->authSecurityPoliciesSize + 1) );
			UA_SecurityPolicy_Basic256Sha256( &config->authSecurityPolicies[config->authSecurityPoliciesSize], *certificate.get(), *privateKey.get(), config->logging );
			config->authSecurityPoliciesSize++;
		}
		config->securityPolicies = securityPolicies.release();
		config->securityPoliciesSize = size;
		config->secureChannelLifeTime = 60 * 60 * 1000;

		return config;
	}
	α UAClient::AddSessionAwait( VoidAwait::Handle h )ι->void{
		{
			lg _{_sessionAwaitableMutex};
			_sessionAwaitables.emplace_back( move(h) );
		}
		Process( std::numeric_limits<RequestId>::max() );
	}
	α UAClient::TriggerSessionAwaitables()ι->void{
		vector<VoidAwait::Handle> handles;
		{
			lg _{_sessionAwaitableMutex};
			for_each(_sessionAwaitables, [&handles](auto&& h){handles.emplace_back(h);} );
			_sessionAwaitables.clear();
		}
		for_each( handles, [](auto&& h){h.resume();} );
	}

	α UAClient::StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void{
		constexpr std::array<sv,6> sessionStates = {"Closed", "CreateRequested", "Created", "ActivateRequested", "Activated", "Closing"};
		DBG( "[{:x}]channelState='{}', sessionState='{}', connectStatus='({:x}){}'", (uint)ua, UAException::Message(channelState), FromEnum(sessionStates, sessionState), connectStatus, UAException::Message(connectStatus) );
		BREAK_IF( connectStatus );
		if( auto client = sessionState == UA_SESSIONSTATE_ACTIVATED ? UAClient::TryFind(ua) : sp<UAClient>{}; client ){
			client->TriggerSessionAwaitables();
			client->ClearRequest( std::numeric_limits<RequestId>::max() );
		}
		if( sessionState == UA_SESSIONSTATE_ACTIVATED || connectStatus==UA_STATUSCODE_BADIDENTITYTOKENINVALID || connectStatus==UA_STATUSCODE_BADCONNECTIONREJECTED || connectStatus==UA_STATUSCODE_BADINTERNALERROR || connectStatus==UA_STATUSCODE_BADUSERACCESSDENIED || connectStatus==UA_STATUSCODE_BADSECURITYCHECKSFAILED || connectStatus == UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
			_awaitingActivation.erase_if( [ua, sessionState,connectStatus]( sp<UAClient> client ){
				if( client->UAPointer()!=ua )return false;
				client->ClearRequest( std::numeric_limits<RequestId>::max() );//previous clear didn't have client
				if( sessionState == UA_SESSIONSTATE_ACTIVATED ){
					{
						ul _{ _clientsMutex };
						client->Connected = true;
						auto& opcCreds = _clients.try_emplace( client->Target() ).first->second;
						let inserted = opcCreds.try_emplace( client->Credential, client ).second;
						ASSERT( inserted );//not sure why we would already have a record.
					}
					Post( [client]()ι->void{ConnectAwait::Resume(move(client));} );
				}
				else
					Post( [client,connectStatus]()ι->void{ConnectAwait::Resume( client->Target(), client->Credential, UAClientException{connectStatus, client->Handle(), "Connection Failed"} );} );

				return true;
			});
		}
	}
	α inactivityCallback( UA_Client* /*client*/ )->void{
		BREAK;
	}
	α subscriptionInactivityCallback( UA_Client *client, SubscriptionId subscriptionId, void* /*subContext*/ ){
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
		if( Credential.Type()==ETokenType::Username )
			UA_ClientConfig_setAuthenticationUsername( &_config, Credential.LoginName().c_str(), Credential.Password().c_str() );
		else if( Credential.Type()==ETokenType::Certificate ){
			UA_ClientConfig_setAuthenticationCert( &_config,
				*ToUAByteString( Crypto::ReadCertificate(AppClient()->ClientCryptoSettings->CertPath) ),
				*ToUAByteString( Crypto::ReadPrivateKey(AppClient()->ClientCryptoSettings->PrivateKeyPath, AppClient()->ClientCryptoSettings->Passcode) )
				//*ToUAByteString( Crypto::ReadCertificate(CertificateFile()) ),
				//*ToUAByteString( Crypto::ReadPrivateKey(PrivateKeyFile(), Passcode()) )
			);
		}else if( Credential.Type()==ETokenType::IssuedToken ){
			UA_IssuedIdentityToken* identityToken = UA_IssuedIdentityToken_new();
			identityToken->policyId = AllocUAString( "open62541-anonymous-policy"sv );
			UA_ByteString_allocBuffer( &identityToken->tokenData, Credential.Token().size() );
			identityToken->tokenData.length = Credential.Token().size();
			memcpy( identityToken->tokenData.data, Credential.Token().data(), Credential.Token().size() );
			UA_ExtensionObject_setValue( &_config.userIdentityToken, identityToken, &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN] );
		}

		//	UA_ClientConfig_setAuthenticationCert( &_config, Credential.Certificate().c_str(), Credential.PrivateKey().c_str() );
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
		DBG( "[{:x}]Connecting to '{}', using '{}'", Handle(), Url(), Credential.ToString() );
		let sc = UA_Client_connectAsync( UAPointer(), Url().c_str() ); THROW_IFX( sc, UAException(sc) );
		_asyncRequest.SetParent( p );
		Process( std::numeric_limits<RequestId>::max(), nullptr );
	}

	α UAClient::Process( RequestId requestId )ι->void{
		_asyncRequest.Process( requestId );
	}

	α UAClient::ProcessDataSubscriptions()ι->void{
		Process( 0 );
	}

	α UAClient::StopProcessDataSubscriptions()ι->void{
		ClearRequest( 0 );
	}

	α UAClient::ClearRequest( UA_Client* ua, RequestId requestId )ι->void{
		if( auto p = TryFind( ua ); p )
			p->ClearRequest( requestId );
	}

	α UAClient::RetryVoid( function<void(sp<UAClient>&&) > f, UAException&& e, sp<UAClient>&& client )ι->ConnectAwait::Task{
		let target = client->Target();
		let credential = client->Credential;
		RemoveClient( move(client) );
		if( e.Code==UA_STATUSCODE_BADCONNECTIONCLOSED || e.Code==UA_STATUSCODE_BADSERVERNOTCONNECTED ){
			try{
				client = co_await GetClient( move(target), move(credential) );
				f( move(client) );
			}
			catch( exception& e ){
			}
		}
	}

	α UAClient::SendBrowseRequest( Browse::Request&& request, Browse::FoldersAwait::Handle h )ι->void{
		try{
			RequestId requestId{};
			TRACET( BrowseTag, "[{:x}]SendBrowseRequest", Handle() );
			UAε( UA_Client_sendAsyncBrowseRequest(_ptr, &request, Browse::OnResponse, nullptr, &requestId) );
			TRACET( BrowseTagPedantic, "[{:x}.{}]SendBrowseRequest", Handle(), requestId );
			Process( requestId, move(h) );
		}
		catch( UAException& e ){
			Retry<Browse::FoldersAwait::Handle>(
				[r{move(request)}]( sp<UAClient>&& p, Browse::FoldersAwait::Handle h )mutable{
					p->SendBrowseRequest( move(r), h );
				},
				move(e), shared_from_this(), h
			);
		}
	}

	α UAClient::SendReadRequest( const flat_set<NodeId>&& nodeIds, ReadValueAwait::Handle h )ι->void{
		if( nodeIds.empty() ){
			h.promise().SetExp( Exception{"no nodes sent"} );
			return h.resume();
		}
		flat_map<UA_UInt32, NodeId> ids;
		RequestId firstRequestId{};
		try{
			//assume 1st will fail if any, request all again if latter node failed.
			for( auto&& nodeId : nodeIds ){
				RequestId requestId{};
				UAε( UA_Client_readValueAttribute_async(_ptr, nodeId, Read::OnResponse, (void*)(uint)firstRequestId, &requestId) );
				if( !firstRequestId )
					firstRequestId = requestId;
				ids.emplace( requestId, nodeId );
	 		}
			TRACET( IotReadTag, "[{:x}.{}]SendReadRequest - count={}", Handle(), firstRequestId, ids.size() );
			_readRequests.try_emplace( firstRequestId, UARequestMulti<Value>{move(ids)} );
			Process( firstRequestId, move(h) );
		}
		catch( UAException& e ){
			Retry<ReadValueAwait::Handle>( [n=move(nodeIds)]( sp<UAClient>&& p, ReadValueAwait::Handle h )mutable{p->SendReadRequest( move(n), move(h) );}, move(e), shared_from_this(), move(h) );
		}
	}
	α UAClient::SendWriteRequest( flat_map<NodeId,Value>&& values, WriteAwait::Handle h )ι->void{
		if( values.empty() ){
			h.promise().ResumeExp( Exception{"no nodes sent"}, move(h) );
			return;
		}
		flat_map<UA_UInt32, NodeId> ids;
		Gateway::RequestId firstRequestId{};
		try{
			for( auto&& [nodeId, value] : values ){
				RequestId requestId{};
				UAε( UA_Client_writeValueAttribute_async(_ptr, nodeId, &value.value, Write::OnResponse, (void*)(uint)firstRequestId, &requestId) );
				if( !firstRequestId )
					firstRequestId = requestId;
				ids.emplace( requestId, nodeId );
	 		}
			_writeRequests.try_emplace( firstRequestId, UARequestMulti<UA_WriteResponse>{move(ids), shared_from_this()} );
			Process( firstRequestId, move(h) );
		}
		catch( UAException& e ){
			Retry<WriteAwait::Handle>( [n=move(values)]( sp<UAClient>&& p, auto h )mutable{p->SendWriteRequest( move(n), move(h) );}, move(e), shared_from_this(), h );
		}
	}
	α UAClient::SetMonitoringMode( Gateway::SubscriptionId subscriptionId )ι->void{
		UA_SetMonitoringModeRequest request{};
		request.subscriptionId = subscriptionId;
		try{
			RequestId requestId{};
			UAε( UA_Client_MonitoredItems_setMonitoringMode_async(UAPointer(), move(request), SetMonitoringModeCallback, nullptr, &requestId) );
			Process( requestId, nullptr );//if retry fails, don't want to process.
		}
		catch( UAException& e ){
			RetryVoid( [subscriptionId]( sp<UAClient>&& p )mutable{p->SetMonitoringMode( subscriptionId );}, move(e), shared_from_this() );
		}
	}

	α UAClient::CreateSubscriptions()ι->void{
		try{
			RequestId requestId{};
			UAε( UA_Client_Subscriptions_create_async(UAPointer(), UA_CreateSubscriptionRequest_default(), nullptr, StatusChangeNotificationCallback, DeleteSubscriptionCallback, CreateSubscriptionCallback, nullptr, &requestId) );
			TRACET( MonitoringTag, "[{:x}.{:x}]CreateSubscription", Handle(), requestId );
			Process( requestId, nullptr );
		}
		catch( UAException& e ){
			RetryVoid( []( sp<UAClient>&& p )mutable{p->CreateSubscriptions();}, move(e), shared_from_this() );
		}
	}

	α UAClient::DataSubscriptions( CreateMonitoredItemsRequest&& request, Jde::Handle requestHandle, DataChangeAwait::Handle h )ι->void{
		ASSERT(CreatedSubscriptionResponse);
		if( !CreatedSubscriptionResponse )
			return h.promise().ResumeExp( Exception{"CreatedSubscriptionResponse==null"}, h );
		try{
			vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbacks{request.itemsToCreateSize, DataChangesDeleteCallback};
			vector<UA_Client_DataChangeNotificationCallback> dataChangeCallbacks{request.itemsToCreateSize, DataChangesCallback};
			void** contexts = nullptr;
			request.subscriptionId = CreatedSubscriptionResponse->subscriptionId;

			RequestId requestId{};
			UAε( UA_Client_MonitoredItems_createDataChanges_async(UAPointer(), request, contexts, dataChangeCallbacks.data(), deleteCallbacks.data(), CreateDataChangesCallback, (void*)requestHandle, &requestId) );
			//TRACET( MonitoringTag, "[{:x}.{:x}]DataSubscriptions - {}", Handle(), requestId, serialize(request.ToJson()) );
			Process( requestId, move(h) );//TODO handle BadSubscriptionIdInvalid
		}
		catch( UAException& e ){
			h.promise().ResumeExp( move(e), move(h) );//retry will leave CreatedSubscriptionResponse null...
			//Retry( [&x=request,requestHandle]( sp<UAClient>&& p, HCoroutine&& h )mutable{p->DataSubscriptions(move(x), requestHandle, move(h));}, move(e), shared_from_this(), move(h) );
		}
	}

	α UAClient::DataSubscriptionDelete( Gateway::SubscriptionId subscriptionId, flat_set<MonitorId>&& monitoredItemIds )ι->void{
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

	α UAClient::RequestDataTypeAttributes( const flat_set<NodeId>&& x, AttribAwait::Handle h )ι->void{
		flat_map<UA_UInt32, NodeId> ids;
		Gateway::RequestId firstRequestId{};
		try{
			for( let& node : x ){
				RequestId requestId{};
				UAε( UA_Client_readDataTypeAttribute_async(_ptr, node, Attributes::OnResponse, (void*)(uint)firstRequestId, &requestId) );
				if( !firstRequestId )
					firstRequestId = requestId;
				ids.emplace( requestId, node );
	 		}
			_dataAttributeRequests.try_emplace( firstRequestId, UARequestMulti<NodeId>{move(ids), shared_from_this()} );
			Process( firstRequestId, move(h) );
		}
		catch( UAException& e ){
			Retry<AttribAwait::Handle>( [n=move(x)]( sp<UAClient>&& p, auto h )mutable{p->RequestDataTypeAttributes( move(n), move(h) );}, move(e), shared_from_this(), h );
		}
	}

	α UAClient::Unsubscribe( const sp<IDataChange>&& dataChange )ι->void{
		sl _{ _clientsMutex };
		for( auto& [_, credClients] : _clients ){
			for( auto& [_, client] : credClients )
				client->MonitoredNodes.Unsubscribe( dataChange );
		}
	}

	α UAClient::BrowsePathsToNodeIds( sv path, bool parents )Ε->flat_map<string,std::expected<ExNodeId,StatusCode>>{
		let segments = Str::Split( path, '/' );
		auto args = Reserve<UABrowsePath>( parents ? segments.size()-1 : 1 );
		vector<string> paths;
		for( uint i=0; i<(parents ? segments.size() : 1); ++i ){
			std::span<const sv> nodePath{ segments.begin(), segments.end()-i };
			paths.emplace_back( Str::Join(nodePath, "/") );
			args.emplace_back( UABrowsePath{nodePath, _opcServer.DefaultBrowseNs} );
		}
		return BrowsePathsToNodeIdResponse{ UA_Client_Service_translateBrowsePathsToNodeIds(_ptr, {{}, args.size(), args.data()}), paths, Handle() };
	}
	α UAClient::Find( UA_Client* ua, SL srce )ε->sp<UAClient>{
		sp<UAClient> y = TryFind( ua, srce );
		if( !y )
	 		throw Exception{ srce, ELogLevel::Debug, "[{}]Could not find client.", format("{:x}", (uint)ua) };
		return y;
	}

	α UAClient::TryFind( UA_Client* ua, SL srce )ι->sp<UAClient>{
		sl _{ _clientsMutex };
		if( Process::ShuttingDown() ){
			LOGSL( ELogLevel::Warning, srce, _tags, "Application is shutting down." );
		}else{
			for( auto& [_, credClients] : _clients ){
				for( auto& [_, client] : credClients ){
					if( client->_ptr == ua )
						return client;
				}
			}
		}
		return {};
	}

	α UAClient::Find( str opcNK, optional<Gateway::Credential> cred )ι->sp<UAClient>{
		sl _{ _clientsMutex };
		sp<UAClient> y;
		if( auto creds = _clients.find(opcNK); creds!=_clients.end() ){
			if( auto client = creds->second.find(*cred); client!=creds->second.end() )
				y = client->second;
		}
		return y;
	}

	UAClient::~UAClient() {
// 		_config.eventLoop->stop( _config.eventLoop );
// #undef free
// 		while( _config.eventLoop->free(_config.eventLoop) ){
// 			if( auto sc = UA_Client_run_iterate(_ptr, 0); sc /*&& (sc!=UA_STATUSCODE_BADINTERNALERROR || i!=0)*/ )
// 				DBG( "UA_Client_run_iterate returned ({:x}){}", sc, UAException::Message(sc) );
// //			std::this_thread::sleep_for( 1s );
// 		}
// 		_config.eventLoop = nullptr;
// #define free _free_dbg
		UA_Client_delete(_ptr);
		DBG("[{:x}]~UAClient( '{}', '{}' )", Handle(), Target(), Url());
	}
}