#include "startup.h"
#include <jde/db/db.h>
#include <jde/access/access.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLHook.h>
#include <jde/opc/opc.h>
#include <jde/opc/uatypes/helpers.h>
#include "ql/OpcServerQL.h"
#include "UAServer.h"
#include "awaits/ServerConfigAwait.h"
#include "ql/ConstructorHook.h"
#include "ql/ObjectTypeHook.h"
#include "ql/ObjectHook.h"

#include "web/WebServer.h"

#define let const auto
namespace Jde::Opc{
	α Server::Startup( std::optional<int>& exitCode )ι->VoidAwait<>::Task{
		try{
			auto authorize = App::Client::RemoteAcl();
			auto uaSchema = DB::GetAppSchema( "opcServer", authorize );
			SetSchema( uaSchema );
			QL::Configure( {uaSchema} );
			Opc::Configure( uaSchema );
			if( auto sync = Settings::FindBool("/dbServers/sync").value_or(true); sync )
				DB::SyncSchema( *uaSchema, QL::Local() );

			Crypto::CryptoSettings settings{ "http/ssl" };
			if( !fs::exists(settings.PrivateKeyPath) ){
				settings.CreateDirectories();
				Crypto::CreateKeyCertificate( settings );
			}
			let serverName = Settings::FindString("/opcServer/target").value_or( "default" );
			let& serverTable = GetViewPtr( "servers" );
			auto serverId = DS().ScalerSyncOpt<uint32>( DB::Statement{
				{serverTable->GetColumnPtr("server_id")},
				{serverTable},
				{serverTable->GetColumnPtr("target"), serverName }
			}.Move() );
			if( !serverId ){
				serverId = DS().InsertSeqSync<uint32>( DB::InsertClause{
					serverTable->InsertProcName(),
					{ {serverName}, {serverName}, {0}, {} }
				} );
			}
			SetServerId( *serverId );

			StartWebServer();
			App::Client::Connect();
			{
				let startTime = Clock::now();
				let connectTimeout = Settings::FindDuration("/appServer/connectTimeout").value_or(5s);
				while( startTime<Clock::now()+connectTimeout && (App::Client::AppClientSocketSession::Instance()==nullptr || App::Client::QLServer()==nullptr) )
					std::this_thread::yield();
				THROW_IF( App::Client::AppClientSocketSession::Instance()==nullptr || App::Client::QLServer()==nullptr, "Could not connect to appServer.  timeout: {}", Chrono::ToString(connectTimeout) );
			}
			auto await = Access::Configure( DB::GetAppSchema("access", authorize), {uaSchema}, App::Client::QLServer(), UserPK{UserPK::System} );
			co_await await;
			GetUAServer().Run();
			co_await ServerConfigAwait{};
			QL::Hook::Add( mu<ConstructorHook>() );
			QL::Hook::Add( mu<ObjectHook>() );
			QL::Hook::Add( mu<ObjectTypeHook>() );
			co_await UpsertAwait{};
			Information( ELogTags::App, "---Started {}---", Process::ProductName() );
		}
		catch( const IException& e ){
			if( e.Level()==ELogLevel::Trace )
				std::cout << e.what() << std::endl;
			else
				std::cerr << e.what() << std::endl;
			Critical( ELogTags::App, "Exiting on error:  {}", e.what() );
			exitCode = e.Code ? (int)e.Code : EXIT_FAILURE;
		}
	}
}