#include "StartupAwait.h"
#include <jde/db/db.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/ql/types/Introspection.h>
#include <jde/access/Authorize.h> //!important
#include <jde/access/AccessListener.h>
#include <jde/access/client/accessClient.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>
#include "opcInternal.h"
#include "UAClient.h"
#include "WebServer.h"
#include "ql/GatewayQL.h"
#include "ql/OpcQLHook.h"

#define let const auto
namespace Jde::Opc{
	static sp<Access::AccessListener> _listener;
}

namespace Jde::Opc::Gateway{
	extern Duration _pingInterval;
	extern Duration _ttl;

	StartupAwait::StartupAwait( jobject webServerSettings, jobject userName, SL sl )ι:
		VoidAwait{sl},
		_webServerSettings{move(webServerSettings)},
		_userName{move(userName)}{
		if( _userName.empty() )
			_userName = jobject{ {"name", Ƒ("OpcGateway-{}", Process::HostName())} };
	}

	α StartupAwait::Execute()ι->VoidAwait::Task{
		try{
			auto authorize = App::Client::RemoteAcl( "gateway" );
			auto schema = DB::GetAppSchema( "gateway", authorize );
			ConfigureQL( {schema}, authorize );
			for( let& path : Settings::FindPathArray("/ql/introspection") )
				QL::AddIntrospection( QL::Introspection{Json::ReadJsonNet(Settings::Directory()/path)} );
			QL::SetSystemTables( {"dataType", "dataTypes", "discoveryUrls", "node","nodes", "securityMode", "securityPolicyUri", "serverDescription", "variable", "variables"} );
			QL::SetSystemMutations( {"execute"} );
			SetSchema( schema );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *schema, QLPtr() );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *schema, QLPtr() );

			Crypto::CryptoSettings sslSettings{ Json::FindDefaultObject(_webServerSettings, "ssl") };
			if( !fs::exists(sslSettings.PrivateKeyPath) ){
				sslSettings.CreateDirectories();
				Crypto::CreateKeyCertificate( sslSettings );
			}
			StartWebServer( move(_webServerSettings) );
			auto accessSchema = DB::GetAppSchema( "access", authorize );
			auto appClient = AppClient();
			appClient->SubscriptionSchemas.push_back( accessSchema );
			appClient->SslSettings = move(sslSettings);
			appClient->SetUserName( move(_userName) );
			co_await App::Client::ConnectAwait{ appClient, false };

			_listener = ms<Access::AccessListener>( appClient->QLServer() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});
			co_await Access::Client::Configure( accessSchema, {schema}, appClient->QLServer(), UserPK{UserPK::System}, authorize, _listener, {} );
			Process::AddShutdownFunction( [](bool terminate){UAClient::Shutdown(terminate);} );
			QL::Hook::Add( mu<OpcQLHook>() );

			_pingInterval = Settings::FindDuration("/gateway/pingInterval").value_or( 60s );
			_ttl = Settings::FindDuration("/gateway/ttl").value_or( 5min );
			INFOT( ELogTags::App, "---Started {}---", "OPC Gateway" );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}