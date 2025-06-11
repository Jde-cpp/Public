#include <iostream>
#include <signal.h>
#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <jde/access/access.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/db/db.h>
#include <jde/opc/opc.h>
#include <jde/opc/uatypes/helpers.h>
#include <jde/opc/uatypes/Node.h>
#include "globals.h"
#include "UAServer.h"
#include "awaits/ServerConfigAwait.h"
#include "web/WebServer.h"

#define let const auto
std::optional<int> _exitCode;
α Jde::Process::ProductName()ι->sv{ return "OpcServer"; }
namespace Jde::Opc::Server{
	Ω startUp( int argc, char **argv )ι->VoidAwait<>::Task;
}
α main( int argc, char **argv )->int{
	using namespace Jde;
	Opc::Server::startUp( argc, argv );
	if( !_exitCode )
		_exitCode = Process::Pause();
	Process::Shutdown( _exitCode.value_or(EXIT_FAILURE) );
	return _exitCode.value_or( EXIT_FAILURE );
}

α Jde::Opc::Server::startUp( int argc, char **argv )ι->VoidAwait<>::Task{
	try{
		TagFromString( Opc::TagFromString );
		TagToString( Opc::TagToString );
		OSApp::Startup( argc, argv, "Jde.OpcGateway", "IOT Connection" );
		auto authorize = App::Client::RemoteAcl();
		auto schema = DB::GetAppSchema( "opcServer", authorize );
		QL::Configure( {schema} );
		Opc::Configure( schema );
		if( auto sync = Settings::FindBool("/dbServers/sync").value_or(true); sync )
			DB::SyncSchema( *schema, QL::Local() );

		Crypto::CryptoSettings settings{ "http/ssl" };
		if( !fs::exists(settings.PrivateKeyPath) ){
			settings.CreateDirectories();
			Crypto::CreateKeyCertificate( settings );
		}

		StartWebServer();
		App::Client::Connect();
		{
			let startTime = Clock::now();
			let connectTimeout = Settings::FindDuration("/appServer/connectTimeout").value_or(5s);
			while( startTime<Clock::now()+connectTimeout && (App::Client::AppClientSocketSession::Instance()==nullptr || App::Client::QLServer()==nullptr) )
				std::this_thread::yield();
			THROW_IF( App::Client::AppClientSocketSession::Instance()==nullptr || App::Client::QLServer()==nullptr, "Could not connect to appServer.  timeout: {}", Chrono::ToString(connectTimeout) );
		}
		auto await = Access::Configure( DB::GetAppSchema("access", authorize), {schema}, App::Client::QLServer(), UserPK{UserPK::System} );
		co_await await;
		auto uaSchema = DB::GetAppSchema( "opcServer", authorize );
		SetSchema( uaSchema );
		SetServerName( Settings::FindString("/opcServer/name").value_or( "default" ) );
		co_await ServerConfigAwait{};
		GetUAServer().Run();
		Information( ELogTags::App, "---Started {}---", Process::ProductName() );
	}
	catch( const IException& e ){
		if( e.Level()==ELogLevel::Trace )
			std::cout << e.what() << std::endl;
		else
			std::cerr << e.what() << std::endl;
		Critical( ELogTags::App, "Exiting on error:  {}", e.what() );
		_exitCode = e.Code ? (int)e.Code : EXIT_FAILURE;
	}
}