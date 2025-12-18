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
#include "WebServer.h"

#define let const auto

namespace Jde::App{
	sp<DB::AppSchema> _appSchema;
	sp<Access::Authorize> _authorizer = ms<Access::Authorize>( "App" );
	sp<Access::AccessListener> _listener;
	constexpr ELogTags _tags{ ELogTags::App };
	Ω ds()ι->DB::IDataSource&{ return *_appSchema->DS(); }
	Ω instanceTableName()ε->string{ return _appSchema->GetView("connections").DBName; }

namespace Server{

	α ConfigureDSAwait::Suspend()ι->void{ Configure(); }
	α ConfigureDSAwait::Configure()ι->VoidAwait::Task{
		try{
			auto accessSchema = DB::GetAppSchema( "access", _authorizer );
			_appSchema = DB::GetAppSchema( "app", _authorizer );
			SetLocalQL( QL::Configure({accessSchema, _appSchema}, _authorizer) );
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
			co_await Access::Server::Configure( {accessSchema, _appSchema}, QLPtr(), UserPK{UserPK::System}, _authorizer, _listener );
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

}
/*
	#define _pQueue if( auto p = _pDbQueue; p )p
	α Server::SaveString( Proto::FromClient::EFields field, StringMd5 id, string value, SL )ι->void{
		sv table = "log_messages";
		if( field==Proto::FromClient::EFields::FileId )
			table = "log_files";
		else if( field==Proto::FromClient::EFields::FunctionId )
			table = "log_functions";
		else if( field!=Proto::FromClient::EFields::MessageId ){
			//ERRX( "unknown field '{}'.", (int)field );
			return;
		}
		DB::Sql sql{ Ƒ( "insert into {}(id,value)values(?,?)", table ) };
		//ASSERT( Calc32RunTime(*pValue)==id );
		//if( Calc32RunTime(value)!=id )
			//return ERRX( "id '{}' does not match crc of '{}'", id, value );//locks itself on server log.
		sql.Params.push_back( {id} );
		sql.Params.push_back( {move(value)} );
	}
*/
}

namespace Jde{
	α App::AddConnection( str appName, str instanceName, str hostName, uint pid )ε->tuple<AppPK, AppInstancePK, AppConnectionPK>{
		AppPK appId{};
		AppInstancePK appInstanceId{};
		AppConnectionPK appConnectionId{};
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
	α App::EndInstance( AppInstancePK instanceId, SL sl )ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().Execute( {Ƒ("update {} set end_time=now() where id=?", instanceTableName()), {DB::Value{instanceId}}}, sl );
		}
		catch( exception& )
		{}
	}

/*
	α App::LoadApplications( AppPK id )ι->up<Proto::FromServer::Applications>
	{
		auto pApplications = mu<Proto::FromServer::Applications>();
		auto fnctn = [&pApplications]( const DB::IRow& row )
		{
			auto pApplication = pApplications->add_values();
			pApplication->set_id( row.GetUInt32(0) );
			pApplication->set_name( row.GetString(1) );
			optional<uint> dbLevel = row.GetUIntOpt( 2 );
			pApplication->set_db_level( dbLevel.has_value() ? (Jde::Proto::ELogLevel)dbLevel.value() : Jde::Proto::ELogLevel::Information );
			optional<uint> fileLevel = row.GetUIntOpt( 3 );
			pApplication->set_file_level( fileLevel.has_value() ? (Jde::Proto::ELogLevel)fileLevel.value() : Jde::Proto::ELogLevel::Information );
		};

		constexpr sv baseSql = "select id, name, db_log_level, file_log_level from log_applications"sv;
		Try( [&](){
			string sql = id ? Ƒ("{} where id=?", baseSql) : string{baseSql};
			let params = id ? vector<DB::object>{id} : vector<DB::object>{};
			if( auto p = Datasource(); p )
				p->Select( sql, fnctn, params );
		} );
		return pApplications;
	}

	α App::SaveMessage( AppPK applicationId, AppInstancePK instanceId, const Log::Proto::LogEntryClient& m, SL )ι->void{
		let variableCount = std::min( 5, m.args().size() );
		vector<DB::Value> params{
			{applicationId},
			{instanceId},
			{m.file_id()},
			{m.function_id()},
			{m.line()},
			{m.message_id()},
			{(uint8)m.level()},
//			{m.thread_id()},
			{Jde::Proto::ToTimePoint(m.time())},
			{m.user_pk()} };
		constexpr sv procedure = "log_message_insert"sv;
		constexpr sv args = "(?,?,?,?,?,?,?,?,?,?"sv;
		std::ostringstream os;
		os << procedure;
		if( variableCount>0 )
			os << variableCount;
		os << args;
		for( int i=0; i<variableCount; ++i ){
			os << ",?";
			params.push_back( {m.args()[i]} );
		}
		os << ")";
	}
*/
}