#include "StartupAwait.h"
#include <jde/db/db.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/ql/QLHook.h>
#include <jde/access/Authorize.h>
#include <jde/access/AccessListener.h>
#include <jde/access/client/accessClient.h>
#include <jde/app/client/appClient.h>
#include <jde/app/client/AppClientSocketSession.h>
#include <jde/opc/uatypes/opcHelpers.h>
#include "OpcServerAppClient.h"
#include "UAServer.h"
#include "awaits/ServerConfigAwait.h"
#include "ql/ConstructorHook.h"
#include "ql/ObjectTypeHook.h"
#include "ql/ObjectHook.h"
#include "ql/OpcServerQL.h"
#include "web/WebServer.h"


#define let const auto
namespace Jde::Opc{
	sp<QL::LocalQL> _localQL;
	sp<Access::AccessListener> _listener;
	α Server::Schemas()ι->const vector<sp<DB::AppSchema>>&{ return _localQL->Schemas(); }
}
namespace Jde::Opc::Server{

	α StartupAwait::Execute()ι->VoidAwait::Task{
		try{
			auto remoteAcl = App::Client::RemoteAcl( "opc" );
			auto uaSchema = DB::GetAppSchema( "opc", remoteAcl );
			_localQL = QL::Configure( {uaSchema}, remoteAcl );
			//Opc::Configure( uaSchema );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *uaSchema, _localQL );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *uaSchema, _localQL );

			Crypto::CryptoSettings settings{ Json::FindDefaultObject(_webServerSettings,"ssl") };
			if( !fs::exists(settings.PrivateKeyPath) ){
				settings.CreateDirectories();
				Crypto::CreateKeyCertificate( settings );
			}
			AppClient()->ClientCryptoSettings = settings;
			let serverName = Settings::FindString("/opcServer/target").value_or( "default" );
			let& serverTable = uaSchema->GetViewPtr( "servers" );
			auto serverId = uaSchema->DS()->ScalerSyncOpt<uint32>( DB::Statement{
				{ serverTable->GetColumnPtr("server_id") },
				{ serverTable },
				{ serverTable->GetColumnPtr("target"), serverName }
			}.Move() );
			if( !serverId ){
				serverId = uaSchema->DS()->InsertSeqSync<uint32>( DB::InsertClause{
					serverTable->InsertProcName(),
					{ {serverName}, {serverName}, {0}, {} }
				} );
			}
			StartWebServer( move(_webServerSettings) );
			auto accessSchema = DB::GetAppSchema( "access", remoteAcl );
			AppClient()->SubscriptionSchemas.push_back( accessSchema );
			AppClient()->SetUserName( move(_userName) );
			co_await App::Client::ConnectAwait{ AppClient(), false };
			_listener = ms<Access::AccessListener>( AppClient()->QLServer() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});
			co_await Access::Client::Configure( accessSchema, {uaSchema}, AppClient()->QLServer(), UserPK{UserPK::System}, remoteAcl, _listener );
			QL::Hook::Add( mu<ConstructorHook>() );
			QL::Hook::Add( mu<ObjectHook>() );
			QL::Hook::Add( mu<ObjectTypeHook>() );
			Initialize( *serverId, uaSchema );
			GetUAServer().Run();
			if( Settings::FindBool("/opcServer/db").value_or(false) )
				co_await ServerConfigAwait{}; //database
			if( Settings::FindPath("/opcServer/mutationsDir") )
				co_await UpsertAwait{}; //mutations
			for( let& config : Settings::FindPathArray("/opcServer/configFiles") )
				GetUAServer().Load( config );
			INFOT( ELogTags::App, "---Started OPC Server---" );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}