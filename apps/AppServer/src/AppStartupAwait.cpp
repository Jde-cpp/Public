#include "AppStartupAwait.h"
#include <jde/web/server/SessionGraphQL.h>
#include <jde/web/server/SettingQL.h>
#include <jde/web/server/SubscribeLog.h>
#include "WebServer.h"
#include "LocalClient.h"
#include "LogData.h"
#include "ql/AppInstanceHook.h"

#define let const auto
namespace Jde::App{
	α Server::InitLogging()ι->void{
		AppClient()->InitLogging();
	}
namespace Server{
	α AppStartupAwait::Execute()ι->VoidAwait::Task{
		try{
			co_await ConfigureDSAwait{};
			str instanceName{ Settings::FindString("/instanceName").value_or(_debug ? "Debug" : "Release") };
			let pks = AddConnection( Process::AppName(), instanceName, Process::HostName(), Process::ProcessId() );
			Logging::Add<Web::Server::SubscribeLog>( "subscribe", get<0>(pks), get<1>(pks) );
			SetAppPKs( pks );

			QL::SetSystemTables( {"apps", "connections"} );
			auto appClient = AppClient();
			appClient->SetPublicKey( Crypto::CryptoSettings{Json::FindDefaultObject(_webServerSettings, "ssl")}.PublicKey() );
			Server::StartWebServer( move(_webServerSettings) );
			QL::Hook::Add( mu<AppInstanceHook>(appClient) );
			QL::Hook::Add( mu<Web::Server::SessionGraphQL>(appClient) );
			INFOT( ELogTags::App, "--AppServer Started.--" );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
			//OSApp::UnPause();
		}
	}
}}