#include "StartupAwait.h"
#include <jde/db/db.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/ql/QLHook.h>
#include <jde/access/Authorize.h>
#include <jde/access/AccessListener.h>
#include <jde/access/client/accessClient.h>
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
	sp<QL::LocalQL> _localQL;
	sp<Access::AccessListener> _listener;
	α Server::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return _localQL->Schemas(); }
}
namespace Jde::Opc::Server{
	sp<Access::Authorize> _authorizer = ms<Access::Authorize>();

	α StartupAwait::Execute()ι->VoidAwait<>::Task{
		try{
			auto remoteAcl = App::Client::RemoteAcl();
			auto uaSchema = DB::GetAppSchema( "opc", remoteAcl );
			SetSchema( uaSchema );
			_localQL = QL::Configure( {uaSchema}, _authorizer );
			Opc::Configure( uaSchema );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *uaSchema, _localQL );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *uaSchema, _localQL );

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

			StartWebServer( move(_webServerSettings) );
			auto accessSchema = DB::GetAppSchema( "access", remoteAcl );
			co_await App::Client::ConnectAwait{ {accessSchema} };
			_listener = ms<Access::AccessListener>( App::Client::QLServer() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});
			co_await Access::Client::Configure( accessSchema, {uaSchema}, App::Client::QLServer(), UserPK{UserPK::System}, _authorizer, _listener );
			GetUAServer().Run();
			co_await ServerConfigAwait{};
			QL::Hook::Add( mu<ConstructorHook>() );
			QL::Hook::Add( mu<ObjectHook>() );
			QL::Hook::Add( mu<ObjectTypeHook>() );
			co_await UpsertAwait{};
			Information( ELogTags::App, "---Started OPC Server---" );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}