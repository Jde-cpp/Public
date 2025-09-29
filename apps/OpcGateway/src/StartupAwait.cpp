#include "StartupAwait.h"
#include <jde/db/db.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/access/Authorize.h>
#include <jde/access/AccessListener.h>
#include <jde/access/client/accessClient.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/IAppClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include "opcInternal.h"
#include "UAClient.h"
#include "WebServer.h"
#include "ql/OpcQLHook.h"

#define let const auto
namespace Jde::Opc{
	static sp<QL::LocalQL> _localQL;
	static sp<Access::AccessListener> _listener;
	α Gateway::QL()ι->QL::LocalQL&{ return *_localQL; }
	α Gateway::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return _localQL->Schemas(); }
}

namespace Jde::Opc::Gateway{
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
			_localQL = QL::Configure( {schema}, authorize );
			SetSchema( schema );
			//Opc::Configure( schema );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *schema, _localQL );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *schema, _localQL );

			Crypto::CryptoSettings settings{ Json::FindDefaultObject(_webServerSettings, "ssl") };
			if( !fs::exists(settings.PrivateKeyPath) ){
				settings.CreateDirectories();
				Crypto::CreateKeyCertificate( settings );
			}
			StartWebServer( move(_webServerSettings) );
			auto accessSchema = DB::GetAppSchema( "access", authorize );
			auto appClient = AppClient();
			appClient->SubscriptionSchemas.push_back( accessSchema );
			appClient->ClientCryptoSettings = move(settings);
			appClient->SetUserName( move(_userName) );
			co_await App::Client::ConnectAwait{ appClient, false };

			_listener = ms<Access::AccessListener>( appClient->QLServer() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});
			co_await Access::Client::Configure( accessSchema, {schema}, appClient->QLServer(), UserPK{UserPK::System}, authorize, _listener );
			Process::AddShutdownFunction( [](bool terminate){UAClient::Shutdown(terminate);} );
			QL::Hook::Add( mu<OpcQLHook>() );

			INFOT( ELogTags::App, "---Started {}---", "OPC Gateway" );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}