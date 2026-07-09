#include <sqlite3.h>
#include "SqliteDataSource.h"
#include <jde/db/DBException.h>
#include <jde/db/generators/Functions.h>
#include "SqliteProcs.h"
#include "SqliteQueryAwait.h"
#include "SqliteRow.h"
#include "SqliteServerMeta.h"
#include "../../src/DBLog.h"

#define let const auto

namespace Jde::DB::Sqlite{
	SqliteDataSource::~SqliteDataSource(){
		if( _db )
			sqlite3_close_v2( _db );
	}

	α SqliteDataSource::SetConfig( const jobject& config )ε->void{
		//{ "driver": ".../Jde.DB.Sqlite.so", "path": ":memory:" | "/var/lib/jde/gateway.db" }
		_path = string{ Json::FindSV(config, "path").value_or(":memory:") };
	}

	α SqliteDataSource::Connection( SL sl )ε->sqlite3&{
		if( !_db ){
			//SQLITE_OPEN_FULLMUTEX (serialized) as a backstop; _connMutex is the real serialization.
			let rc = sqlite3_open_v2( _path.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr );
			THROW_IFSL( rc!=SQLITE_OK, "sqlite3_open_v2('{}') failed: {}", _path, sqlite3_errstr(rc) );
			sqlite3_exec( _db, "pragma foreign_keys=on", nullptr, nullptr, nullptr );
			if( _path!=":memory:" )
				sqlite3_exec( _db, "pragma journal_mode=wal", nullptr, nullptr, nullptr );
		}
		return *_db;
	}

	α SqliteDataSource::Disconnect()ε->void{
		lg l{ _connMutex };
		if( _db ){
			sqlite3_close_v2( _db );
			_db = nullptr;
		}
	}

	//"call app_instance_insert( ?, ?, ?, ? )" never reaches the server - there is none. Dispatch to the
	//native twin registered in SqliteProcs, wrapped in a transaction so multi-statement procs stay atomic.
	α SqliteDataSource::ExecuteProc( DB::Sql& sql, SL sl, Params& exeParams )ε->uint{
		let name = Str::RTrim( sv{sql.Text}.substr(0, sql.Text.find('(')) );
		let proc = FindProc( name );
		THROW_IFSL( !proc, "No native proc registered for '{}'.", name );
		auto& db = Connection( sl );
		ExecuteStatement( db, "begin immediate", {}, nullptr, sl );
		try{
			//MySql passes out params as trailing placeholders; the native proc returns them as a row instead - drop them here.
			vector<Value> params{ sql.Params.begin(), sql.Params.end()-(exeParams.HasOut() ? 1 : 0) };
			let y = (*proc)( db, params, exeParams.Function, sl );
			ExecuteStatement( db, "commit", {}, nullptr, sl );
			return y;
		}
		catch( ... ){
			ExecuteStatement( db, "rollback", {}, nullptr, sl );
			throw;
		}
	}

	α SqliteDataSource::Execute( DB::Sql&& sql, SL sl, Params exeParams )ε->uint{
		if( exeParams.Log )
			DB::Log( sql, sl );
		lg l{ _connMutex };
		if( sql.IsProc )
			return ExecuteProc( sql, sl, exeParams );

		auto& db = Connection( sl );
		let y = ExecuteStatement( db, sql.Text, sql.Params, exeParams.Function, sl );
		return exeParams.Sequence ? (uint)sqlite3_last_insert_rowid( &db ) : y;
	}

	α SqliteDataSource::ExecuteSync( Sql&& sql, SL sl )ε->uint{
		return Execute( move(sql), sl );
	}
	α SqliteDataSource::ExecuteNoLog( Sql&& sql, SL sl )ε->uint{
		return Execute( move(sql), sl, {.Log=false} );
	}
	α SqliteDataSource::ExecuteScalerSync( Sql&& sql, EValue outValue, SL sl )ε->Value{
		Value y;
		RowΛ f = [&]( Row&& r )->void{
			THROW_IFSL( r.Size()==0, "Query did not return any {}.", empty(outValue) ? "rows" : "out params" );
			y = move( r[0] );
		};
		Execute( move(sql), sl, {&f, outValue} );
		return y;
	}

	α SqliteDataSource::InsertSeqSyncUInt( DB::InsertClause&& insert, SL sl )ε->uint{
		//Plain inserts: last_insert_rowid covers it - no out param needed, unlike MySql.
		return Execute( insert.Move(), sl, {.Sequence=true} );
	}

	α SqliteDataSource::Select( Sql&& s, SL sl )ε->vector<Row>{
		vector<Row> rows;
		RowΛ f = [&rows]( Row&& r ){ rows.push_back( move(r) ); };
		Execute( move(s), sl, {&f} );
		return rows;
	}
	α SqliteDataSource::Select( Sql&& s, RowΛ f, SL sl )ε->uint{
		return Execute( move(s), sl, {&f} );
	}

	α SqliteDataSource::Query( Sql&& sql, bool outParams, SL sl )ε->QueryAwait{
		return QueryAwait{ mu<SqliteQueryAwait>(dynamic_pointer_cast<SqliteDataSource>(shared_from_this()), move(sql), outParams, sl), sl };
	}

	α SqliteDataSource::AtCatalog( sv, SL sl )ε->sp<IDataSource>{
		LOGSL( ELogLevel::Critical, sl, _tags, "Sqlite doesn't have catalogs." );
		return shared_from_this();
	}
	α SqliteDataSource::AtSchema( sv schema, SL sl )ε->sp<IDataSource>{
		THROW_IFSL( schema!="main", "Sqlite schema '{}' not supported - only 'main'.", schema ); //TODO: ATTACH DATABASE if multiple schemas needed.
		return shared_from_this();
	}

	α SqliteDataSource::ServerMeta()ι->IServerMeta&{
		if( !_serverMeta )
			_serverMeta = mu<SqliteServerMeta>( shared_from_this() );
		return *_serverMeta;
	}
}
Jde::DB::IDataSource* GetDataSource(){
	return new Jde::DB::Sqlite::SqliteDataSource();
}
