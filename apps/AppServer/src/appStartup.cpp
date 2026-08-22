#include "appStartup.h"
#include <jde/fwk/co/Await.h>
#include <jde/db/db.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/access/server/accessServer.h>
#include <jde/access/Authorize.h>
#include <jde/access/AccessListener.h>
#include <jde/web/server/SessionGraphQL.h>
#include <jde/web/server/SettingQL.h>
#include <jde/web/server/SubscribeLog.h>
#include "WebServer.h"
#include "LocalClient.h"
#include "ql/AppInstanceHook.h"
#include "ql/AppServerQL.h"

#define let const auto
namespace Jde::App{
	static sp<DB::AppSchema> _appSchema;
	Ω ds()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	Ω connectionTableName()ε->string{ return _appSchema->GetView("connections").DBName; }
	//ends every connection still open - the ones earlier runs left behind.  ConfigureDS calls it once, before AddConnection.
	Ω endAppInstances()ε->void{
		ds().ExecuteSync( {Ƒ("update {} set deleted={} where deleted is null", connectionTableName(), ds().Syntax().UtcNow())} );
	}

	α AddConnection( str appName, str instanceName, str hostName, uint pid )ε->tuple<ProgramPK, ProgInstPK, ConnectionPK>{
		ProgramPK appId{};
		ProgInstPK appInstanceId{};
		ConnectionPK appConnectionId{};
		let rows = ds().Select( {
			Ƒ("{}(?,?,?,?)", _appSchema->GetTable("connections").InsertProcName()),
			{DB::Value{appName}, {instanceName}, DB::Value{hostName}, DB::Value{pid}},
			true} );
		for( auto&& row : rows ){
			appId = row.GetUInt32(0);
			appInstanceId = row.GetUInt32(1);
			appConnectionId = row.GetUInt32(2);
		}

		return make_tuple( appId, appInstanceId, appConnectionId );
	}
	α EndConnection( ConnectionPK connectionId, SL sl )ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().Execute( {Ƒ("update {} set deleted={} where connection_id=? and deleted is null", connectionTableName(), ds().Syntax().UtcNow()), {DB::Value{connectionId}}}, sl );
		}
		catch( runtime_error& )
		{}
	}
}

namespace Jde::App::Server{
	static sp<Access::AccessListener> _listener;

	α InitLogging()ι->void{
		AppClient()->InitLogging();
	}
	α AppSchema()ι->sp<DB::AppSchema>{ return _appSchema; }

	Ω configureDS()ε->void{
		auto authorizer = Authorizer();
		auto accessSchema = DB::GetAppSchema( "access", authorizer );
		_appSchema = DB::GetAppSchema( "app", authorizer );

		ConfigureQL( {accessSchema, _appSchema}, authorizer );
		_listener = ms<Access::AccessListener>( QLPtr() );
		Process::AddShutdownFunction( []( bool terminate, SL sl ){
			_listener->Shutdown( terminate, sl );
			_listener = nullptr;
		});

		if( Settings::FindBool("/testing/recreateDB").value_or(false) ){
			DB::NonProd::Recreate( *accessSchema, QLPtr() );
			DB::NonProd::Recreate( *_appSchema, QLPtr() );
		}
		else if( Settings::FindBool("/dbServers/sync").value_or(false) || accessSchema->DS()->RequiresSync() ){
			DB::SyncSchema( *accessSchema, QLPtr() );
			DB::SyncSchema( *_appSchema, QLPtr() );
		}
		BlockVoidAwait( Access::Server::Configure({accessSchema, _appSchema}, QLPtr(), UserPK{UserPK::System}, authorizer, _listener) );//the access load is a coroutine chain; this is the sync api over it.
		endAppInstances();
	}

	α AppStartup( jobject webServerSettings )ε->void{
		configureDS();
		str instanceName{ Settings::FindString("/instanceName").value_or(_debug ? "Debug" : "Release") };
		let pks = AddConnection( Process::AppName(), instanceName, Process::HostName(), Process::ProcessId() );
		Logging::Add<Web::Server::SubscribeLog>( "subscribe", get<0>(pks), get<1>(pks) );
		SetAppPKs( pks );

		QL::SetSystemTables( {"apps", "connections", "logSetting"} );
		auto appClient = AppClient();
		auto sslSettings = Crypto::CryptoSettings{ Json::FindDefaultObject(webServerSettings, "ssl") };
		StartWebServer( move(webServerSettings) );
		appClient->SetPublicKey( sslSettings.PublicKey.Value(SRCE_CUR) );
		appClient->LoadLogSettings();
		QL::Hook::Add( mu<AppInstanceHook>(appClient) );
		QL::Hook::Add( mu<Web::Server::SessionGraphQL>(appClient, Authorizer()) );
		INFOT( ELogTags::App, "--AppServer Started.--" );
	}
}
