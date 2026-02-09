#include "LogData.h"
#include <jde/app/StringCache.h>
#include <jde/app/proto/app.FromServer.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/db.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/ql/ql.h>
#include <jde/ql/LocalQL.h>
#include <jde/access/server/accessServer.h>
#include <jde/access/Authorize.h>
#include <jde/access/AccessListener.h>
#include "LocalClient.h"
#include "WebServer.h"
#include "ql/AppServerQL.h"

#define let const auto

namespace Jde::App{
	sp<DB::AppSchema> _appSchema;
	sp<Access::AccessListener> _listener;
	constexpr ELogTags _tags{ ELogTags::App };
	Ω ds()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	Ω instanceTableName()ε->string{ return _appSchema->GetView("connections").DBName; }

namespace Server{
	α ConfigureDSAwait::Suspend()ι->void{ Configure(); }
	α ConfigureDSAwait::Configure()ι->VoidAwait::Task{
		try{
			auto authorizer = Authorizer();
			auto accessSchema = DB::GetAppSchema( "access", authorizer );
			_appSchema = DB::GetAppSchema( "app", authorizer );

			ConfigureQL( {accessSchema, _appSchema}, authorizer );
			_listener = ms<Access::AccessListener>( QLPtr() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});

			if( Settings::FindBool("/testing/recreateDB").value_or(false) ){
				DB::NonProd::Recreate( *accessSchema, QLPtr() );
				DB::NonProd::Recreate( *_appSchema, QLPtr() );
			}
			else if( Settings::FindBool("/dbServers/sync").value_or(false) ){
				DB::SyncSchema( *accessSchema, QLPtr() );
				DB::SyncSchema( *_appSchema, QLPtr() );
			}
			co_await Access::Server::Configure( {accessSchema, _appSchema}, QLPtr(), UserPK{UserPK::System}, authorizer, _listener );
			EndAppInstances();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ConfigureDSAwait::EndAppInstances()ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().Execute( {Ƒ("update {} set deleted={} where deleted is null", instanceTableName(), ds().Syntax().UtcNow())} );
			Resume();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

}}

namespace Jde{
	α App::AddConnection( str appName, str instanceName, str hostName, uint pid )ε->tuple<ProgramPK, ProgInstPK, ConnectionPK>{
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
	α App::EndInstance( ProgInstPK instanceId, SL sl )ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().Execute( {Ƒ("update {} set end_time=now() where id=?", instanceTableName()), {DB::Value{instanceId}}}, sl );
		}
		catch( exception& )
		{}
	}
}