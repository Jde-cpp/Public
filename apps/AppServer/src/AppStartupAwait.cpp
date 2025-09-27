#include "AppStartupAwait.h"
#include "WebServer.h"
#include "graphQL/AppInstanceHook.h"
#include <jde/web/server/SettingQL.h>
#include "ExternalLogger.h"
#include "LogData.h"
#include <jde/web/server/SessionGraphQL.h>

#define let const auto
namespace Jde::App::Server{
	α AppStartupAwait::Execute()ι->VoidAwait::Task{
		try{
			Logging::Entry::SetGenerator( []( sv text ){ return Crypto::CalcMd5(text); } );
			co_await ConfigureDSAwait{};
			SetAppPKs( AddInstance("Main", Process::HostName(), Process::ProcessId()) );

			Data::LoadStrings();
			auto appClient = AppClient();
			appClient->SetPublicKey( Crypto::CryptoSettings{Json::FindDefaultObject(_webServerSettings, "ssl")}.PublicKey() );
			Server::StartWebServer( move(_webServerSettings) );
			QL::Hook::Add( mu<AppInstanceHook>(appClient) );
			QL::Hook::Add( mu<Web::Server::SettingQL>(appClient) );
			QL::Hook::Add( mu<Web::Server::SessionGraphQL>(appClient) );
			Logging::AddLogger( mu<ExternalLogger>(appClient) );

			Information( ELogTags::App, "--AppServer Started.--" );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
			//OSApp::UnPause();
		}
	}
}