#include "LogData.h"
#include <jde/app/shared/StringCache.h>
#include <jde/app/shared/proto/App.FromServer.h>
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
	sp<DB::AppSchema> _logSchema;
	sp<Access::Authorize> _authorizer = ms<Access::Authorize>( "App" );
	sp<Access::AccessListener> _listener;
	constexpr ELogTags _tags{ ELogTags::App };
	Ω ds()ι->DB::IDataSource&{ return *_logSchema->DS(); }
	Ω instanceTableName()ε->string{ return _logSchema->GetView("app_instances").DBName; }

namespace Server{

	α ConfigureDSAwait::Suspend()ι->void{ Configure(); }
	α ConfigureDSAwait::Configure()ι->VoidAwait::Task{
		try{
			auto accessSchema = DB::GetAppSchema( "access", _authorizer );
			_logSchema = DB::GetAppSchema( "log", _authorizer );
			SetLocalQL( QL::Configure({accessSchema, _logSchema}, _authorizer) );
			_listener = ms<Access::AccessListener>( QLPtr() );
			Process::AddShutdownFunction( []( bool terminate ){
				_listener->Shutdown( terminate );
				_listener = nullptr;
			});

			if( Settings::FindBool("/testing/recreateDB").value_or(false) ){
				DB::NonProd::Recreate( *accessSchema, QLPtr() );
				DB::NonProd::Recreate( *_logSchema, QLPtr() );
			}
			else if( Settings::FindBool("/dbServers/sync").value_or(false) ){
				DB::SyncSchema( *accessSchema, QLPtr() );
				DB::SyncSchema( *_logSchema, QLPtr() );
			}
			co_await Access::Server::Configure( {accessSchema, _logSchema}, QLPtr(), UserPK{UserPK::System}, _authorizer, _listener );
			EndAppInstances();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α ConfigureDSAwait::EndAppInstances()ι->DB::ExecuteAwait::Task{
		try{
			co_await ds().Execute( {Ƒ("update {} set end_time={} where end_time is null", instanceTableName(), ds().Syntax().UtcNow())} );
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
	α App::AddInstance( str applicationName, str hostName, uint processId )ε->std::tuple<AppPK, AppInstancePK>{
		AppPK applicationId{};
		AppInstancePK applicationInstanceId{};
		let rows = ds().Select( {
			Ƒ("{}(?,?,?)", _logSchema->GetTable("app_instances").InsertProcName()),
			{DB::Value{applicationName}, DB::Value{hostName}, DB::Value{processId}},
			true} );
		for( auto&& row : rows ){
			applicationId = row.GetUInt32(0);
			applicationInstanceId = row.GetUInt32(1);
		}

		return make_tuple( applicationId, applicationInstanceId );
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
	namespace App{
		α Data::LoadEntries( QL::TableQL table )ε->Proto::FromServer::Traces{
			auto statement = QL::SelectStatement( table, true );
			if( !statement )
				return {};
			flat_map<LogPK,Proto::FromServer::Trace> mapTraces;
			constexpr uint limit = 1000;
			statement->Limit( limit );
			auto where = statement->Where;
			auto rows = ds().Select( statement->Move() ); //TODO awaitable
			for( auto& row : rows ){
				auto t=FromServer::ToTrace( move(row), table.Columns );
				auto id = t.id();
				mapTraces.emplace( id, move(t) );
			}
			if( mapTraces.size() ){
				constexpr sv variableSql = "select log_id, value, variable_index from log_variables join logs on logs.id=log_variables.log_id";
				if( mapTraces.size()==limit ){
					where.Add( "logs.id<?" );
					where.Params().push_back( {mapTraces.rbegin()->first} );
				}
				auto rows = ds().Select( {Ƒ("{}\n{}\norder by log_id, variable_index", variableSql, where.Move()), where.Params()} );
				for( auto&& row : rows ){
					let id = row.GetUInt32( 0 );
					if( auto pTrace = mapTraces.find(id); pTrace!=mapTraces.end() )
						*pTrace->second.add_args() = move( row.GetString(1) );
				}
			}
			Proto::FromServer::Traces traces;
			for( auto& [id,trace] : mapTraces )
				*traces.add_values() = move(trace);
			return traces;
		}

		Ω loadStrings( str tableName, SRCE )ε->concurrent_flat_map<uuid,string>{
			let& table = _logSchema->GetTablePtr( tableName );
			DB::Statement statement{ table->Columns, DB::FromClause{DB::Join{table->GetPK(), _logSchema->GetTablePtr("entries")->GetPK(), true}}, {} };
			auto rows = ds().Select( {statement.Move()}, sl );
			concurrent_flat_map<uuid,string> map;
			for( auto&& row : rows )
				map.emplace( row.GetGuid(0), move(row.GetString(1)) );
			return map;
		}
		Ω loadFiles( SL sl )ε->concurrent_flat_map<uuid,string>{
			return loadStrings( "source_files", sl );
		}
		Ω loadFunctions( SL sl )ε->concurrent_flat_map<uuid,string>{
			return loadStrings( "source_functions", sl );
		}
		Ω loadMessages( SL sl )ε->concurrent_flat_map<uuid,string>{
			return loadStrings( "messages", sl );
		}

		α Data::LoadStrings( SL sl )ε->void{
			StringCache::Merge( loadFiles(sl), loadFunctions(sl), loadMessages(sl) );
		}
	}
}