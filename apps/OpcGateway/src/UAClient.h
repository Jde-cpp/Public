#pragma once
//#include "../exports.h"
#include <jde/opc/uatypes/Logger.h>
#include <jde/opc/uatypes/ExNodeId.h>
#include "async/AsyncRequest.h"
#include "async/ConnectAwait.h"
#include "async/DataChanges.h"
#include "async/ReadValueAwait.h"
#include "auth/OpcServerSession.h"
#include "types/ServerCnnctn.h"
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
		UAClient( ServerCnnctn&& opcServer, Credential cred )ε;
		UAClient( str address, Credential cred )ε;
		~UAClient();

		operator UA_Client* ()ι{ return _ptr; }
		Ω Shutdown( bool terminate )ι->VoidAwait::Task;
		Ω GetClient( string id, Credential cred, SRCE )ι{ return ConnectAwait{move(id), move(cred), sl}; }
		Ω Find( str id, const Gateway::Credential& cred )ι->sp<UAClient>;
		Ω Find( UA_Client* ua, SRCE )ε->sp<UAClient>;
		Ω TryFind( UA_Client* ua, SRCE )ι->sp<UAClient>;
		Ω RemoveClient( sp<UAClient>&& client )ι->bool;

		α SubscriptionId()Ι->SubscriptionId{ return CreatedSubscriptionResponse ? CreatedSubscriptionResponse->subscriptionId : 0;}

		Ω ClearRequest( UA_Client* ua, RequestId requestId )ι->void;
		Ṫ ClearRequestH( UA_Client* ua , RequestId requestId )ι->T;
		α ClearRequest( RequestId requestId )ι->void{ _asyncRequest.Clear( requestId ); }
		Ŧ ClearRequestH( RequestId requestId )ι->T;//{ return ClearRequest<UARequest<T>>( requestId )->CoHandle; }
		α MonitoredNodes()ι->UAMonitoringNodes&{ if( !_monitoredNodes ) _monitoredNodes = mu<UAMonitoringNodes>(shared_from_this()); return *_monitoredNodes; }
		Ŧ Retry( function<void(sp<UAClient>&&, T)> f, UAException&& e, sp<UAClient> pClient, T h )ι->ConnectAwait::Task;
		α RetryVoid( function<void(sp<UAClient>&&) > f, UAException&& e, sp<UAClient>&& pClient )ι->ConnectAwait::Task;
		α Process( RequestId requestId, sv what )ι->void;
		α Processing()ι->bool{ return _asyncRequest.IsRunning(); }
		α ProcessDataSubscriptions()ι->void;
		α StopProcessDataSubscriptions()ι->void;
		α AddSessionAwait( VoidAwait::Handle h )ι->void;
		α TriggerSessionAwaitables()ι->void;

		α Target()Ι->const ServerCnnctnNK&{ return _opcServer.Target; }
		α Url()Ι->str{ return _opcServer.Url; }
		α IsDefault()Ι->bool{ return _opcServer.IsDefault; }
		α DefaultBrowseNs()Ι->NsIndex{ return _opcServer.DefaultBrowseNs; }
		α Handle()Ι->Jde::Handle{ return _handle;}
		α UAPointer()Ι->UA_Client*{return _ptr;}
		α BrowsePathsToNodeIds( sv path, bool parents )Ε->flat_map<string,std::expected<ExNodeId,StatusCode>>;
		sp<UA_SetMonitoringModeResponse> MonitoringModeResponse;
		sp<UA_CreateSubscriptionResponse> CreatedSubscriptionResponse;
		UA_ClientConfig _config{};//TODO move private.
		Gateway::Credential Credential;
		bool Connected{};
	private:
		Ω Unsubscribe( const sp<IDataChange>&& dataChange )ι->void;
		Ω StateCallback( UA_Client *ua, UA_SecureChannelState channelState, UA_SessionState sessionState, StatusCode connectStatus )ι->void;
		α Configuration()ε->UA_ClientConfig*;
		α Create()ι->UA_Client*;
		α Connect()ε->void;
		Ω LogServerEndpoints( str url, Jde::Handle h )ι->void;
		α LogClientEndpoints()ι->void;
		α RootSslDir()ι->fs::path{ return Process::AppDataFolder()/"ssl"; }
		α Passcode()ι->const string{ return Process::GetEnv("JDE_PASSCODE").value_or( "" ); }
		α PrivateKeyFile()ι->fs::path{ return RootSslDir()/Ƒ("private/{}.pem", Target()); }
		α CertificateFile()ι->fs::path{ return RootSslDir()/Ƒ("certs/{}.pem", Target()); }

		ServerCnnctn _opcServer;

		vector<VoidAwait::Handle> _sessionAwaitables; mutable mutex _sessionAwaitableMutex;

		AsyncRequest _asyncRequest;
		Jde::Handle _handle;
		Logger _logger; //after handle
		UA_Client* _ptr{};//needs to be after _logger, _config, Password.
		friend ConnectAwait;
		friend α Read::OnResponse( UA_Client *client, void *userdata, RequestId requestId, StatusCode status, UA_DataValue *value )ι->void;
		friend α Write::OnResponse( UA_Client *ua, void *userdata, RequestId requestId, UA_WriteResponse *response )ι->void;
		friend α Attributes::OnResponse( UA_Client* ua, void* userdata, RequestId requestId, StatusCode status, UA_NodeId* dataType )ι->void;

		up<UAMonitoringNodes> _monitoredNodes;//destroy first
	};

#define let const auto
	Ŧ UAClient::Retry( function<void(sp<UAClient>&&, T)> f, UAException&& e, sp<UAClient> client, T h )ι->ConnectAwait::Task{
		//TODO limit retry attempts.
		let target = client->Target();
		let credential = client->Credential;
		RemoveClient( move(client) );
		if( e.Code==UA_STATUSCODE_BADCONNECTIONCLOSED || e.Code==UA_STATUSCODE_BADSERVERNOTCONNECTED ){
			try{
				client = co_await GetClient( move(target), move(credential) );
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