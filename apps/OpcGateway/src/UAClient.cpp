#include "UAClient.h"

#include <open62541/plugin/securitypolicy_default.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/collections.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>
#include "StartupAwait.h"
#include "async/DataChanges.h"
#include "async/SetMonitoringMode.h"
#include "uatypes/Browse.h"
#include "uatypes/CreateMonitoredItemsRequest.h"
#include "uatypes/uaTypes.h"

#define let const auto

namespace Jde::Opc::Gateway{
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
	concurrent_flat_map<uint32_t, uint32_t> _handles;
	α createHandle( const ServerCnnctn& target )ι->Jde::Handle{
		uint32_t serverHash = target.Id ? target.Id : std::hash<string>{}( target.Url );
		uint32_t connectionIndex{};
		auto increment = [&connectionIndex]( auto& last ){ connectionIndex = ++last.second; };
		_handles.try_emplace_and_visit( serverHash, 0, increment, increment );
		return ( (Jde::Handle)serverHash << 32 ) | connectionIndex;
	}

	concurrent_flat_set<sp<UAClient>> _awaitingActivation;

	UAClient::UAClient( str address, Gateway::Credential cred )ε:
		UAClient{ ServerCnnctn{address}, move(cred) }
	{}

	UAClient::UAClient( ServerCnnctn&& opcServer, Gateway::Credential cred )ε:
		Credential{ move(cred) },
		_opcServer{ move(opcServer) },
		_handle{ createHandle(_opcServer) },
		_logger{ _handle },
		_ptr{ Create() }{
		UA_ClientConfig_setDefault( Configuration() );
		INFO( "[{:x}]Creating UAClient target: '{}' url: '{}' credential: '{}' )", Handle(), Target(), Url(), Credential.ToString() );
		LogClientEndpoints();
	}

	α UAClient::Shutdown( bool /*terminate*/ )ι->VoidAwait::Task{
		{
			sl _1{ _clientsMutex };
			for( auto& [_,creds] : _clients ){
				for( auto& [_,client] : creds ){
					if( auto p = move(client->_monitoredNodes); p )
						co_await p->Shutdown();
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
		let uri = Str::Replace( _opcServer.CertificateUri, " ", "%20" );
		bool addSecurity = !uri.empty();//urn:JDE-CPP:Kepware.KEPServerEX.V6:UA%20Server
		auto certAuth = Credential.Type()==ETokenType::Certificate;
		//TODO - test no security also
		if( addSecurity && !certAuth && !fs::exists(CertificateFile()) ){
			if( !fs::exists(root) )
				fs::create_directories( root );
			const string passcode = Passcode();
			if( !fs::exists(privateKeyFile) )
				Crypto::CreateKey( root/Ƒ("public/{}.pem", Target()), privateKeyFile, passcode );
			Crypto::CreateCertificate( CertificateFile(), privateKeyFile, passcode, Ƒ("URI:{}", uri), "jde-cpp", "US", "localhost" );
		}
		auto config = UA_Client_getConfig( _ptr );
		using SecurityPolicyPtr = up<UA_SecurityPolicy, decltype( &UA_free )>;
		const uint size = addSecurity ? 2 : 1; ASSERT( !config->securityPoliciesSize );
		SecurityPolicyPtr securityPolicies{ (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy)*size), &UA_free };
		auto sc = UA_SecurityPolicy_None( &securityPolicies.get()[0], UA_BYTESTRING_NULL, &_logger ); THROW_IFX( sc, UAClientException(sc, Handle()) );
		if( addSecurity ){
			config->applicationUri = UA_STRING_ALLOC( uri.c_str() );
			config->clientDescription.applicationUri = UA_STRING_ALLOC( uri.c_str() );
			let& settings = AppClient()->SslSettings; //requires authentication[AppClient] & transport[OpcServer] security be equal.
			certAuth = certAuth && settings.has_value();
			let certificateFile = certAuth ? settings->CertPath : CertificateFile();
			let privateKeyFile = certAuth ? settings->PrivateKeyPath : PrivateKeyFile();
			let passcode = certAuth ? settings->Passcode : Passcode();
			INFO( "[{}]Using Basic256Sha256 security policy with certificate '{}'", hex(Handle()), certificateFile.string() );
			auto certificate = ToUAByteString( Crypto::ReadCertificate(certificateFile) );
			auto privateKey = ToUAByteString( Crypto::ReadPrivateKey(privateKeyFile, passcode) );
			sc = UA_SecurityPolicy_Basic256Sha256( &securityPolicies.get()[1], *certificate, *privateKey, &_logger ); THROW_IFX( sc, UAClientException(sc, Handle()) );

			config->authSecurityPolicies = ( UA_SecurityPolicy * )UA_realloc( config->authSecurityPolicies, sizeof(UA_SecurityPolicy) *(config->authSecurityPoliciesSize + 1) );
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
			lg _{ _sessionAwaitableMutex };
			_sessionAwaitables.emplace_back( move(h) );
		}
		Process( ConnectRequestId, "Connect" );
	}
	α UAClient::TriggerSessionAwaitables()ι->void{
		vector<VoidAwait::Handle> handles;
		{
			lg _{ _sessionAwaitableMutex };
			for_each( _sessionAwaitables, [&handles](auto&& h){handles.emplace_back(h);} );
			_sessionAwaitables.clear();
		}
		for_each( handles, [](auto&& h){h.resume();} );
	}

	α UAClient::LogClientEndpoints()ι->void{
		vector<string> policyUris;
		auto config = UA_Client_getConfig( _ptr );
		for( let& sp : Iterable<UA_SecurityPolicy>( config->securityPolicies, config->securityPoliciesSize) )
			policyUris.emplace_back( ToString(sp.policyUri) );
		INFO( "[{:x}]Client Security Policies: {}", Handle(), Str::Join(policyUris) );
	}
	α UAClient::LogServerEndpoints( str url, Jde::Handle h )ι->void{
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
		UA_EndpointDescription* endpointArray{}; uint endpointArraySize{};

		if( UA_Client_getEndpoints(client, url.c_str(), &endpointArraySize, &endpointArray) ){
			WARN( "[{}]Could not get endpoints for url='{}'", hex(h), url );
		}
		else{
			for( auto&& ep : Iterable<UA_EndpointDescription>(endpointArray, endpointArraySize) ){
				constexpr array<sv,4> securityModeNames = { "Invalid", "None", "Sign", "SignAndEncrypt" };
				let securityMode = FromEnum( securityModeNames, ep.securityMode );
				vector<string> tokenTypes;
				for( uint j=0; j<ep.userIdentityTokensSize; ++j )
					tokenTypes.emplace_back( FromEnum(TokenTypeNames, ToTokenType(ep.userIdentityTokens[j].tokenType)) );
				INFO( "[{}]ServerEndpoint {}=[{}]", hex(h), securityMode, Str::Join(tokenTypes) );
			}
			UA_Array_delete( endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION] );
		}
		UA_Client_delete( client );
	}

	α UAClient::StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void{
		constexpr std::array<sv,6> sessionStates = { "Closed", "CreateRequested", "Created", "ActivateRequested", "Activated", "Closing" };
		DBG( "[{:x}]channelState='{}', sessionState='{}', connectStatus='({:x}){}'", (uint)ua, UAException::Message(channelState), FromEnum(sessionStates, sessionState), connectStatus, UAException::Message(connectStatus) );
		if( auto client = sessionState == UA_SESSIONSTATE_ACTIVATED ? UAClient::TryFind(ua) : sp<UAClient>{}; client ){
			client->TriggerSessionAwaitables();
			client->ClearRequest( ConnectRequestId );
		}
		if( sessionState == UA_SESSIONSTATE_ACTIVATED || connectStatus==UA_STATUSCODE_BADIDENTITYTOKENINVALID || connectStatus==UA_STATUSCODE_BADCONNECTIONREJECTED || connectStatus==UA_STATUSCODE_BADINTERNALERROR || connectStatus==UA_STATUSCODE_BADUSERACCESSDENIED || connectStatus==UA_STATUSCODE_BADSECURITYCHECKSFAILED || connectStatus == UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
			_awaitingActivation.erase_if( [ua, sessionState,connectStatus](sp<UAClient> client){
				if( client->UAPointer()!=ua )return false;

				if( connectStatus == UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
					LogServerEndpoints( client->Url(), client->Handle() );
					client->LogClientEndpoints();
				}
				client->ClearRequest( ConnectRequestId );//previous clear didn't have client
				if( sessionState == UA_SESSIONSTATE_ACTIVATED ){
					{
						ul _{ _clientsMutex };
						client->Connected = true;
						auto& opcCreds = _clients.try_emplace( client->Target() ).first->second;
						let inserted = opcCreds.try_emplace( client->Credential, client ).second;
						ASSERT( inserted ); // not sure why we would already have a record.
					}
					Post( [client]()ι->void {
						ConnectAwait::Resume(move(client));
					});
				}
				else
					Post( [client,connectStatus]()ι->void {ConnectAwait::Resume(client->Target(), client->Credential, UAClientException{connectStatus, client->Handle(), "Connection Failed"});} );

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
		_config.eventLoop->registerEventSource( _config.eventLoop, (UA_EventSource*)tcpCM );
		_config.timeout = 10000; /*ms*/
		_config.stateCallback = StateCallback;
		_config.inactivityCallback = inactivityCallback;
		_config.subscriptionInactivityCallback = subscriptionInactivityCallback;
		if( Credential.Type()==ETokenType::Username ){
			UA_ClientConfig_setAuthenticationUsername( &_config, Credential.LoginName().c_str(), Credential.Password().c_str() );
			INFO( "[{}]Using username/password authentication: '{}'", hex(Handle()), Credential.LoginName() );
		}else if( Credential.Type()==ETokenType::Certificate ){
			let& settings = AppClient()->SslSettings;
			let& certPath = settings->CertPath;
			INFO( "[{}]Using certificate authentication: '{}'", hex(Handle()), certPath.string() );
			UA_ClientConfig_setAuthenticationCert( &_config,
				*ToUAByteString( Crypto::ReadCertificate(certPath) ),
				*ToUAByteString( Crypto::ReadPrivateKey(settings->PrivateKeyPath, settings->Passcode) )
			);
		}else if( Credential.Type()==ETokenType::IssuedToken ){
			ASSERT( Credential.Token().size() );
			INFO( "[{}]Using issued token authentication: '{}'", hex(Handle()), Credential.Token() );
			UA_IssuedIdentityToken* identityToken = UA_IssuedIdentityToken_new();
			identityToken->policyId = AllocUAString( "open62541-anonymous-policy"sv );
			UA_ByteString_allocBuffer( &identityToken->tokenData, Credential.Token().size() );
			identityToken->tokenData.length = Credential.Token().size();
			memcpy( identityToken->tokenData.data, Credential.Token().data(), Credential.Token().size() );
			UA_ExtensionObject_setValue( &_config.userIdentityToken, identityToken, &UA_TYPES[UA_TYPES_ISSUEDIDENTITYTOKEN] );
		}
		else{
			BREAK;
			INFO( "[{}]Using anonymous authentication.", hex(Handle()) );
		}

		//	UA_ClientConfig_setAuthenticationCert( &_config, Credential.Certificate().c_str(), Credential.PrivateKey().c_str() );
		UA_ConnectionManager *udpCM = UA_ConnectionManager_new_POSIX_UDP( "udp connection manager"_uv );
		_config.eventLoop->registerEventSource( _config.eventLoop, (UA_EventSource*)udpCM );
		auto ua = UA_Client_newWithConfig( &_config );
		UA_Client_getConfig( ua )->eventLoop->logger = _config.logging;

		return ua;
	}
	α UAClient::Connect()ε->void{
		auto p = shared_from_this();
		ASSERT( !_awaitingActivation.contains(p) );
		_awaitingActivation.emplace( shared_from_this() );
		DBG( "[{:x}]Connecting to '{}', using '{}'", Handle(), Url(), Credential.ToString() );
		let sc = UA_Client_connectAsync( UAPointer(), Url().c_str() ); THROW_IFX( sc, UAException(sc) );
		_asyncRequest.SetClient( p );
		Process( ConnectRequestId, "Connect" );
	}

	α UAClient::Process( RequestId requestId, sv what )ι->void{
		_asyncRequest.Process( requestId, what );
	}

	α UAClient::ProcessDataSubscriptions()ι->void{
		Process( SubscriptionRequestId, "DataSubscriptions" );
	}

	α UAClient::StopProcessDataSubscriptions()ι->void{
		ClearRequest( SubscriptionRequestId );
	}

	α UAClient::ClearRequest( UA_Client* ua, RequestId requestId )ι->void{
		if( auto p = TryFind(ua); p )
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

	α UAClient::Unsubscribe( const sp<IDataChange>&& dataChange )ι->void{
		sl _{ _clientsMutex };
		for( auto& [_, credClients] : _clients ){
			for( auto& [_, client] : credClients )
				client->MonitoredNodes().Unsubscribe( dataChange );
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
		if( Process::ShuttingDown() ){
			LOGSL( ELogLevel::Warning, srce, _tags, "Application is shutting down." );
		}else{
			sl _{ _clientsMutex };
			for( auto& [_, credClients] : _clients ){
				for( auto& [_, client] : credClients ){
					if( client->_ptr == ua )
						return client;
				}
			}
		}
		return {};
	}

	α UAClient::Find( str opcNK, const Gateway::Credential& cred )ι->sp<UAClient>{
		sl _{ _clientsMutex };
		sp<UAClient> y;
		if( auto creds = _clients.find(opcNK); creds!=_clients.end() ){
			if( auto client = creds->second.find(cred); client!=creds->second.end() )
				y = client->second;
		}
		return y;
	}

	UAClient::~UAClient() {
		UA_Client_delete( _ptr );
		DBG( "[{:x}]~UAClient( '{}', '{}' )", Handle(), Target(), Url() );
	}
}