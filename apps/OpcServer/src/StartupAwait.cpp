#include "StartupAwait.h"
#include <jde/db/db.h>
#include <jde/ql/QLHook.h>
#include <jde/access/AccessListener.h>
#include <jde/access/client/accessClient.h>
#include <jde/app/client/appClient.h>
#include "OpcServerAppClient.h"
#include "UAServer.h"
#include "access/OpcAuthorize.h"
#include "awaits/ServerConfigAwait.h"
#include "ql/ConstructorHook.h"
#include "ql/ObjectTypeHook.h"
#include "ql/ObjectHook.h"
#include "ql/OpcQL.h"
#include "ql/OpcServerQL.h"
#include "web/WebServer.h"


#define let const auto
namespace Jde::Opc::Server{
	α StartupAwait::Execute()ι->VoidAwait::Task{
		sp<OpcAuthorize> opcAuthorize;
		{
			auto schemaSuffix = Settings::FindString( "/opcServer/resource" );
			App::Client::SetAcl( opcAuthorize=ms<OpcAuthorize>(schemaSuffix ? "opc." + *move(schemaSuffix) : "opc") );
		}
		try{
			auto remoteAcl = App::Client::RemoteAcl( "opc" );
			auto uaSchema = DB::GetAppSchema( "opc", remoteAcl );
			uaSchema->Authorizer = opcAuthorize;// GetAppSchema returns a cached schema whose Authorizer is baked in when GetClusters first builds the cache. When another server (e.g. embedded AppServer) built it first with a base Access::Authorize, our SetAcl(OpcAuthorize) above is ignored here. Install it explicitly so UAAccess::GetUserAccessLevel's static_cast<OpcAuthorize&> is valid (and so UserRights reads the same _nodeResources that AssignRights populates).
			ConfigureQL( uaSchema, remoteAcl );
			QL::SetSystemTables( {"logSetting"} );
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *uaSchema, QLPtr() );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *uaSchema, QLPtr() );

			Crypto::CryptoSettings settings{ Json::FindDefaultObject(_webServerSettings,"ssl") };
			if( !fs::exists(settings.PrivateKeyPath) ){
				settings.CreateDirectories();
				Crypto::CreateKeyCertificate( settings );
			}
			auto appClient = AppClient();
			appClient->SslSettings = settings;
			let serverName = Settings::FindString( "/opcServer/target" ).value_or( "default" );
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
			StartWebServer( move(_webServerSettings) ); //TODO take out.
			auto accessSchema = DB::GetAppSchema( "access", remoteAcl );
			appClient->SubscriptionSchemas.push_back( accessSchema );

			auto resourceSchema = Settings::FindString( "/opcServer/resource" ).value_or( "" );
			appClient->ResourceSchema = resourceSchema.size() ? Ƒ( "opc.{}", resourceSchema ) : "opc";

			appClient->SetUserName( move(_userName) );
			co_await App::Client::ConnectAwait{ appClient, false };
			appClient->LoadLogSettings();

			co_await Access::Client::Configure( accessSchema, {uaSchema}, appClient->QLServer(), UserPK{UserPK::System}, remoteAcl, appClient->Listener(), resourceSchema );
			QL::Hook::Add( mu<ConstructorHook>() );
			QL::Hook::Add( mu<ObjectHook>() );
			QL::Hook::Add( mu<ObjectTypeHook>() );
			Initialize( *serverId, uaSchema );
			auto& ua = GetUAServer();
			ua.Run();
			if( Settings::FindBool("/opcServer/db").value_or(false) )
				co_await ServerConfigAwait{}; //database
			if( Settings::FindPath("/opcServer/mutationsDir") )
				co_await UpsertAwait{}; //mutations
			for( let& config : Settings::FindPathArray("/opcServer/configFiles") )
				ua.Load( config );

			opcAuthorize->AssignRights( ua );
			for( let& [idx, ns] : ua.Namespaces() )
				INFOT( ELogTags::App, "ns: {}, uri: {}", idx, ns );

			INFOT( ELogTags::App, "---Started OPC Server---" );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}