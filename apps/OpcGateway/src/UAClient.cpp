#include "UAClient.h"

#include <open62541/plugin/securitypolicy_default.h>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/utils/collections.h>
#include <jde/app/client/IAppClient.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>
#include <open62541/types.h>
#include <stdexcept>
#include "StartupAwait.h"
#include "async/DataChanges.h"
#include "jde/fwk/crypto/CryptoSettings.h"
#include "jde/fwk/settings.h"
#include "uatypes/Browse.h"
#include "uatypes/uaTypes.h"

#define let const auto

namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	flat_map<ServerCnnctnNK,flat_map<Credential,sp<UAClient>>> _clients; shared_mutex _clientsMutex;
	α UAClient::RemoveClient( sp<UAClient>&& client )ι->bool{
		client->Connected = false;
		client->StopProcessing();//cancels the ping timer & processing loop; otherwise _pingTimer stays pending on the io_context (and the ping coroutine keeps a UAClient ref), blocking shutdown.
		bool erased{};
		ul _{ _clientsMutex };
		if( auto serverCreds = _clients.find(client->Target()); serverCreds!=_clients.end() ){
			if( auto cred = serverCreds->second.find(client->Credential); cred!=serverCreds->second.end() ){
				DBG( "[{}]Removing client: '{}'.", hex(client->Handle()), client->Target() );
				serverCreds->second.erase( cred );
				erased = true;
				if( serverCreds->second.empty() )
					_clients.erase( serverCreds );

			}
		}
		if( !erased )
			DBG( "[{}] - could not find client='{}'.", hex(client->Handle()), client->Target() );
		client = nullptr;
		return erased;
	}
	α UAClient::StatusCounts()ι->tuple<uint,uint>{
		vector<sp<UAClient>> clients;
		{
			sl _{ _clientsMutex };
			for( let& [target, creds] : _clients )
				for( let& [cred, client] : creds )
					clients.push_back( client );
		}
		uint monitored{};//count outside the lock - MonitoredNodes() lazily constructs and Count() takes the nodes mutex.
		for( let& client : clients )
			monitored += client->MonitoredNodes().Count();
		return { clients.size(), monitored };
	}
	concurrent_flat_map<uint32_t, uint32_t> _handles;
	α createHandle( const ServerCnnctn& target )ι->Jde::Handle{
		//Handle packs the server id into its top 32 bits, so fold the 64-bit hash rather than truncating it (xor high^low keeps more entropy). A collision only merges two servers' connection-index counters, which are purely for log correlation - benign.
		uint32_t serverHash = target.Id ? target.Id : []( size_t h )ι{ return (uint32_t)h ^ (uint32_t)(h>>32); }( std::hash<string>{}(target.Url) );
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
		try{
			//Configuration() must run BEFORE setDefault: it installs the custom security policies (and asserts securityPoliciesSize==0 first). setDefault then only back-fills fields left unset — its own None-policy install is guarded by securityPoliciesSize==0, so it won't clobber ours (open62541 ua_config_default.c). Reversing the order would trip Configuration()'s assert and leak the default policy.
			let sc = UA_ClientConfig_setDefault( Configuration() ); THROW_IFX( sc, UAClientException(sc, Handle()) );
			INFO( "[{}]Creating UAClient target: '{}' url: '{}' credential: '{}' )", hex(Handle()), Target(), Url(), Credential.ToString() );
			LogClientEndpoints();
		}
		catch( ... ){
			UA_Client_delete( _ptr );
			_ptr = nullptr;
			throw;
		}
	}

	α UAClient::Shutdown( bool /*terminate*/, SL /*sl*/ )ι->VoidAwait::Task{
		vector<sp<UAClient>> clients;
		{
			sl _1{ _clientsMutex };
			for( auto&& [_,creds] : _clients )
				for( auto&& [_,client] : creds )
					clients.push_back( client );
		}
		//Must not hold _clientsMutex across co_await: the awaitable resumes on another pool thread (UB to unlock a shared_mutex off-thread) and the resume path (RemoveClient, StateCallback insertion) needs the unique lock.
		for( auto& client : clients ){
			if( auto p = move(client->_monitoredNodes); p )
				co_await p->Shutdown();
			client->StopProcessing();
			if( client.use_count()>2 )//_clients + this local copy are expected; more may just be pending strand closures (StopProcessing above) - log-only heuristic.
				WARN( "[{}]use_count={}", hex(client->Handle()), client.use_count() );
		}
		ul _{ _clientsMutex };
		_clients.clear();
	}

	//for soak
	α UAClient::CryptoSettings( const ServerCnnctnNK& target, sv certificateUri )ι->Crypto::CryptoSettings{
		auto settings = Settings::FindDefaultObject( "/gateway/issuedCerts" );//a copy - the SAN below is per-target.
		if( certificateUri.size() ){
			//the client cert's SAN uri is what a server matches against the applicationUri we advertise (Configuration()
			//sets both from the target's certificateUri), so it cannot come from a single config-wide constant.
			auto certificate = Json::FindDefaultObject( settings, "certificate" );
			certificate["subjectAltName"] = Ƒ( "URI:{}", Str::Replace(string{certificateUri}, " ", "%20") );
			settings["certificate"] = move( certificate );
		}
		return Crypto::CryptoSettings{ settings, target };
	}

	//the file name keys on the target but the SAN on the certificateUri, so an existing file is not proof it is the
	//cert this config describes; a changed uri would otherwise be rejected as BadCertificateUriInvalid forever with
	//nothing naming the file to delete.  SanUri only, never the whole SubjectAltName: the der ctor rebuilds that by
	//joining sanEntry() renderings, and any lossy round-trip would mismatch on every connect and re-issue in a loop.
	Ω certificateMatches( const Crypto::CryptoSettings& settings, SL sl )ε->bool{
		if( !fs::exists(settings.Certificate.Path) )
			return false;
		let onDisk = Crypto::Certificate{ Crypto::ReadCertificate(settings.Certificate.Path), sl };//an unreadable cert throws, as it does in EnsureKeyCertificate - keep the two policies together.
		let matches = onDisk.SanUri()==settings.Certificate.SanUri();
		if( !matches )
			INFO( "Re-issuing '{}': certificateUri '{}' -> '{}'.", settings.Certificate.Path.string(), onDisk.SanUri(), settings.Certificate.SanUri() );
		return matches;
	}

	α UAClient::EnsureCertificate( const ServerCnnctnNK& target, sv uri, SL sl )ε->void{
		let& settings = CryptoSettings( target, uri );
		if( certificateMatches(settings, sl) )
			return;
		settings.CreateDirectories();
		if( !fs::exists(settings.PrivateKey.Path) )
			Crypto::CreateKey( settings, sl );
		Crypto::IssueCertificate( settings, sl );
	}
	α UAClient::Configuration()ε->UA_ClientConfig*{
		let uri = Str::Replace( _opcServer.CertificateUri, " ", "%20" );
		bool addSecurity = !uri.empty();
		auto certAuth = Credential.Type()==ETokenType::Certificate;
		//TODO - test no security also
		if( addSecurity && !certAuth )
			EnsureCertificate( Target(), uri );
		auto config = UA_Client_getConfig( _ptr );
		const uint size = addSecurity ? 2 : 1; ASSERT( !config->securityPoliciesSize );
		uint initialized = 0;//policies actually constructed; on an exception before ownership transfers to config, the deleter clears these — UA_free alone would leak each policy's internals (policyUri, contexts, ...).
		auto clearPolicies = [&initialized]( UA_SecurityPolicy* p )ι{ for(uint i=0; i<initialized; ++i) p[i].clear(&p[i]); UA_free(p); };
		up<UA_SecurityPolicy, decltype( clearPolicies )> securityPolicies{ (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy)*size), clearPolicies };
		auto sc = UA_SecurityPolicy_None( &securityPolicies.get()[0], UA_BYTESTRING_NULL, &_logger ); THROW_IFX( sc, UAClientException(sc, Handle()) );
		++initialized;
		if( addSecurity ){
			UA_String_clear( &config->applicationUri );//clear any existing value before overwriting so the default isn't leaked.
			config->applicationUri = UA_STRING_ALLOC( uri.c_str() );
			UA_String_clear( &config->clientDescription.applicationUri );
			config->clientDescription.applicationUri = UA_STRING_ALLOC( uri.c_str() );
			certAuth = certAuth && AppClient()->SslSettings.has_value();
			let& settings = certAuth ? *AppClient()->SslSettings : CryptoSettings(); //requires authentication[AppClient] & transport[OpcServer] security be equal.
			INFO( "[{}]Using Basic256Sha256 security policy with certificate '{}'", hex(Handle()), settings.Certificate.Path.string() );
			auto certificate = ToUAByteString( Crypto::ReadCertificate(settings.Certificate.Path) );
			auto privateKey = ToUAByteString( Crypto::ReadPrivateKey(settings.PrivateKey) );
			sc = UA_SecurityPolicy_Basic256Sha256( &securityPolicies.get()[1], *certificate, *privateKey, &_logger ); THROW_IFX( sc, UAClientException(sc, Handle()) );
			++initialized;

			auto grown = ( UA_SecurityPolicy* )UA_realloc( config->authSecurityPolicies, sizeof(UA_SecurityPolicy) *(config->authSecurityPoliciesSize + 1) );
			THROW_IFX( !grown, UAClientException(UA_STATUSCODE_BADOUTOFMEMORY, Handle()) );//realloc failure leaves the original block valid; don't overwrite the pointer with null (would leak it and null-deref below).
			config->authSecurityPolicies = grown;
			sc = UA_SecurityPolicy_Basic256Sha256( &config->authSecurityPolicies[config->authSecurityPoliciesSize], *certificate.get(), *privateKey.get(), config->logging ); THROW_IFX( sc, UAClientException(sc, Handle()) );
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

	α UAClient::ApplicationUri()Ι->string{
		return ToString( UA_Client_getConfig(_ptr)->applicationUri );
	}
	α UAClient::LogClientEndpoints()ι->void{
		vector<string> policyUris;
		auto config = UA_Client_getConfig( _ptr );
		for( let& sp : Iterable<UA_SecurityPolicy>(config->securityPolicies, config->securityPoliciesSize) )
			policyUris.emplace_back( ToString(sp.policyUri) );
		//both uris: config->applicationUri filters the *server's* endpoints, clientDescription's is what we advertise -
		//they come from the same certificateUri (Configuration()) and a mismatch in either rejects every endpoint.
		INFO( "[{}]Client Security Policies: {}, applicationUri filter: '{}', advertised applicationUri: '{}'", hex(Handle()), Str::Join(policyUris), ToString(config->applicationUri), ToString(config->clientDescription.applicationUri) );
	}
	//returns the server's ApplicationUri so the caller can name both sides of an endpoint-filter mismatch.
	α UAClient::LogServerEndpoints( str url, Jde::Handle h )ι->string{
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig( client );
    UA_ClientConfig_setDefault( config );
		UA_EndpointDescription* endpointArray{}; uint endpointArraySize{};
		string serverUri;

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
				let applicationUri = ToString( ep.server.applicationUri );
				INFO( "[{}]ServerEndpoint {}=[{}], applicationUri: '{}'", hex(h), securityMode, Str::Join(tokenTypes), applicationUri );
				if( serverUri.empty() )
					serverUri = applicationUri;//every endpoint of one server carries the same uri; keep the first non-empty.
			}
			UA_Array_delete( endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION] );
		}
		UA_Client_delete( client );
		return serverUri;
	}

	α UAClient::StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void{
		constexpr std::array<sv,6> sessionStates = { "Closed", "CreateRequested", "Created", "ActivateRequested", "Activated", "Closing" };
		DBG( "[{}]channelState: '{}', sessionState: '{}', connectStatus: '({}){}'", hex((uint)ua), UAException::Message(channelState), FromEnum(sessionStates, sessionState), hex(connectStatus), UAException::Message(connectStatus) );
		if( auto client = sessionState == UA_SESSIONSTATE_ACTIVATED ? UAClient::TryFind(ua) : sp<UAClient>{}; client ){
			if constexpr( _debug )
				client->Process( PingRequestId, "ping" ); //mitigate No result in the read namespace array response
			client->TriggerSessionAwaitables();
			client->ClearRequest( ConnectRequestId );
		}

		if( sessionState == UA_SESSIONSTATE_ACTIVATED || connectStatus ){
			_awaitingActivation.erase_if( [ua, sessionState,connectStatus](sp<UAClient> client){
				if( client->UAPointer()!=ua )return false;

				string detail;//the reason handed to the waiting requests, not just the log - empty falls back to "Connection Failed".
				if( connectStatus == UA_STATUSCODE_BADIDENTITYTOKENREJECTED ){
					let serverUri = LogServerEndpoints( client->Url(), client->Handle() );
					client->LogClientEndpoints();
					//open62541 reports "No suitable endpoint found" as BadIdentityTokenRejected, so the usual cause - our
					//configured applicationUri filtering out every endpoint (matchEndpoint) - reads as a credential problem.
					//Name both uris; the fix is the target's certificateUri, which is what Configuration() puts in the filter.
					if( let clientUri = client->ApplicationUri(); clientUri.size() && !serverUri.empty() && clientUri!=serverUri ){
						detail = Ƒ( "client applicationUri '{}' does not match the server's '{}' at '{}' - every endpoint is filtered out; correct the target's certificateUri", clientUri, serverUri, client->Url() );
						ERR( "[{}]{}", hex(client->Handle()), detail );
					}
				}
				else if( auto sslSettings=connectStatus==UA_STATUSCODE_BADCERTIFICATEINVALID ? AppClient()->SslSettings : optional<Crypto::CryptoSettings>{}; sslSettings ){
					detail = Ƒ( "certificate '{}' rejected", sslSettings->Certificate.Path.string() );//path only - ToString() carries the whole subject/issuer/SAN dump, which belongs in the log.
					ERR( "Certificate: {} rejected.", sslSettings->Certificate.ToString() );
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
						ConnectAwait::Resume( move(client) );
					});
				}
				else{
					client->StopProcessing();// Break the UAClient<->_asyncRequest._client self-reference; on the failure path the client never enters _clients, so Shutdown would never Stop() it and the UAClient (and its UA_Client) would leak.
					Post( [client,connectStatus,detail=move(detail)]()ι->void {ConnectAwait::Resume(client->Target(), client->Credential, UAClientException{connectStatus, client->Handle(), detail.size() ? detail : "Connection Failed"});} );
				}

				return true;
			});
		}
	}
	α inactivityCallback( UA_Client* /*client*/ )->void{
		BREAK;
	}
	α subscriptionInactivityCallback( UA_Client *client, SubscriptionId subscriptionId, void* /*subContext*/ ){
		DBG( "[{}.{}]subscriptionInactivityCallback", hex((uint)client), hex(subscriptionId) );
	}
	α UAClient::Create()ε->UA_Client*{
		//Every throwing step stays ABOVE the first allocation.  Create() is called from UAClient's member-initializer
		//list, so a throw out of it escapes before the object exists: ~UAClient never runs, and nothing else knows
		//about the event loop, the connection managers or the UA_Client.  Reading the credential material first means
		//the only failure that can realistically happen (missing/unreadable cert) leaves nothing to clean up.
		ByteStringPtr certificate{ nullptr, UA_ByteString_delete };
		ByteStringPtr privateKey{ nullptr, UA_ByteString_delete };
		if( Credential.Type()==ETokenType::Certificate ){
			//never blind-deref: Configuration() guards the same optional, and cert auth without ssl settings is a
			//configuration error, not a crash.
			let& ssl = AppClient()->SslSettings; THROW_IF( !ssl, "[{}]Certificate authentication configured but the app client has no ssl settings.", hex(Handle()) );
			certificate = ToUAByteString( Crypto::ReadCertificate(ssl->Certificate.Path) );
			privateKey = ToUAByteString( Crypto::ReadPrivateKey(ssl->PrivateKey) );
		}
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
			INFO( "[{}]Using certificate authentication: '{}'", hex(Handle()), AppClient()->SslSettings->Certificate.ToString() );
			UA_ClientConfig_setAuthenticationCert( &_config, *certificate, *privateKey );//read above, before anything was allocated.
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
			BREAK; //need to sign in.
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
		//Pre-concurrency: nothing drives this client's run_iterate until Process below starts the loop, so the direct
		//UA_Client_connectAsync/SetClient calls here are single-threaded. Process must stay the LAST statement - after
		//it, every UA_Client_* call must go through the strand (PostUA).
		DBG( "[{}]Connecting to '{}', using '{}'", hex(Handle()), Url(), Credential.ToString() );
		let sc = UA_Client_connectAsync( UAPointer(), Url().c_str() ); THROW_IFX( sc, UAException(sc) );
		auto p = shared_from_this();
		ASSERT( !_awaitingActivation.contains(p) );
		_awaitingActivation.emplace( shared_from_this() );
		_asyncRequest.SetClient( p );
		Process( ConnectRequestId, "Connect" );
	}

	α UAClient::PostUA( function<void()> f )ι->void{
		//All UA_Client_* calls must run on this client's strand (open62541 clients are not thread-safe): run_iterate,
		//async submissions, and sync services all serialize here. dispatch runs f inline when the caller is already on
		//the strand (e.g. completion callbacks inside run_iterate) and posts otherwise. `self` keeps the client - and
		//with it _asyncRequest and the raw UA_Client - alive until f runs.
		boost::asio::dispatch( _asyncRequest.Strand(), [self=shared_from_this(), f=move(f)]{f();} );
	}

	α UAClient::PostStrand( function<void()> f )ι->void{
		//Always post (never dispatch inline): the handler runs after the current strand op returns. Used to break
		//re-entrancy - e.g. resuming a caller from inside run_iterate must not unblock/destroy the awaitable while the
		//strand handler that drove run_iterate is still touching it. `self` keeps the client (and its strand) alive until f runs.
		boost::asio::post( _asyncRequest.Strand(), [self=shared_from_this(), f=move(f)]{f();} );
	}

	α UAClient::Process( RequestId requestId, sv what )ι->void{
		if( _asyncRequest.IsStopped() )
			return;
		PostUA( [this, requestId, what=string{what}]{_asyncRequest.Process(requestId, what);} );
	}

	α UAClient::ClearRequest( RequestId requestId )ι->void{
		PostUA( [this, requestId]{_asyncRequest.Clear(requestId);} );
	}

	α UAClient::StopProcessing()ι->void{
		PostUA( [this]{_asyncRequest.Stop();} );
	}

	α UAClient::ShutdownIdle( sp<UAClient> client )ι->VoidAwait::Task{
		//TTL expiry (called from the processing loop): tear down only this client - _lastRequest is per-client, so one
		//client idling out must not disconnect the others (Shutdown() stops everything).
		if( client->_monitoredNodes )//not moved out: data-change callbacks can still arrive until RemoveClient below.
			co_await client->_monitoredNodes->Shutdown();
		RemoveClient( move(client) );
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
		if( e.Code()==UA_STATUSCODE_BADCONNECTIONCLOSED || e.Code()==UA_STATUSCODE_BADSERVERNOTCONNECTED ){
			try{
				client = co_await GetClient( move(target), move(credential) );
				f( move(client) );
			}
			catch( UAException& retryEx ){
				retryEx.PrependWhat( "Retry failed" );
				retryEx.SetLevel( ELogLevel::Warning );
			}
		}
	}

	α UAClient::Unsubscribe( const sp<IDataChange>&& dataChange )ι->void{
		sl _{ _clientsMutex };
		for( auto&& [_, credClients] : _clients ){
			for( auto&& [_, client] : credClients )
				client->MonitoredNodes().Unsubscribe( dataChange );
		}
	}

	α UAClient::BrowsePathsToNodeIds( sv path, bool parents )Ε->flat_map<string,ExpectedNodeId>{
		ASSERT( _asyncRequest.Strand().running_in_this_thread() );//sync UA service - must be serialized with run_iterate (route callers through PostUA/UAStrandAwait).
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
	 		throw Exception{ srce, ELogLevel::Debug, "[{}]Could not find client.", hex((uint)ua) };
		return y;
	}

	α UAClient::TryFind( UA_Client* ua, SL srce )ι->sp<UAClient>{
		if( Process::ShuttingDown() ){
			LOGSL( ELogLevel::Warning, srce, _tags, "Application is shutting down." );
		}else{
			sl _{ _clientsMutex };
			for( auto&& [_, credClients] : _clients ){
				for( auto&& [_, client] : credClients ){
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
		INFO( "[{}]~UAClient( '{}', '{}' )", hex(Handle()), Target(), Url() );
	}
}