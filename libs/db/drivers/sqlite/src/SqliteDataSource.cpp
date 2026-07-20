#include "SqliteDataSource.h"
#include "jde/fwk/exceptions/Exception.h"
#include "jde/fwk/process/dll.h"
#include <jde/db/DBException.h>
#include <jde/db/generators/Functions.h>
#include "SqliteException.h"
#include "SqliteProcs.h"
#include "SqliteQueryAwait.h"
#include "SqliteRow.h"
#include "SqliteServerMeta.h"
#include "../../../src/DBLog.h"

#define let const auto

namespace Jde::DB::Sqlite{
	//A loaded proc dll. Extends ProcRegistry so the dll registers through us; we record the names it registers and
	//unregister them in the destructor - before _dll unloads - so their ProcΛ std::functions (bodies in the dll)
	//aren't destroyed after dlclose (which would fault during the registry's static teardown at exit).
	class SqliteApi final : public ProcRegistry{
		DllHelper _dll;
		vector<string> _procNames;
	public:
		SqliteApi( fs::path path ): _dll{ move(path) }{
			decltype(RegisterProcs)* registerProcs{ _dll["RegisterProcs"] };
			registerProcs( *this );
		}
		~SqliteApi(){ UnregisterProcs( _procNames ); } //while _dll is still mapped (destroyed after this body).

		α RegisterProc( string name, ProcΛ proc, uint minParams )ι->void override{
			_procNames.push_back( name );
			ProcRegistry::RegisterProc( move(name), move(proc), minParams );
		}
	};

	//One SqliteApi per dll, shared process-wide: the proc registry is global, so two data sources configured with the
	//same dll register the same names - a per-data-source SqliteApi would unregister them when either was destroyed,
	//stripping procs the survivor still dispatches. Weak entries so the dll still unloads with its last data source.
	DllApiCache<SqliteApi> _dllApis;

	SqliteDataSource::~SqliteDataSource(){
		if( _db )
			sqlite3_close_v2( _db );
	}

	α SqliteDataSource::SetConfig( const jobject& config )ε->void{
		for( auto&& [catalogName, vcatalog] : Json::AsObject(config, "catalogs") ){
			let& catalog = Json::AsObject( vcatalog );
			if( let path = Json::FindSV(catalog, "path") )
				_path = *path; //defaults to ':memory:' when no catalog supplies a path.
			for( auto&& [dbSchemaName, dbSchema] : Json::AsObject(catalog, "schemas") ){
				if( dbSchemaName.starts_with('_') ) //internal schema, not a real db schema.
					continue;
				for( auto&& [appSchemaName, vappSchema] : Json::AsObject(dbSchema) ){
					let lib = Json::FindSV( Json::AsObject(vappSchema), "dynamicLib" );
					THROW_IFX( !lib, Exception(SRCE_CUR, {ELogLevel::Critical, ELogTags::App}, "No dynamicLib for {}.{}.{}", catalogName, dbSchemaName, appSchemaName) );
					fs::path dynamicLib{ *lib };
					if( !_procDlls.contains(dynamicLib) ){
						auto api = _dllApis.Get( dynamicLib ); //ctor loads the dll and registers its procs.
						_procDlls.emplace( move(dynamicLib), move(api) );
					}
				}
			}
		}
	}

	α SqliteDataSource::Connection( SL sl )ε->sqlite3&{
		if( !_db ){
			//SQLITE_OPEN_FULLMUTEX (serialized) as a backstop; _connMutex is the real serialization.
			let rc = sqlite3_open_v2( _path.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr );
			THROW_IFX( rc, SqliteException( sl, rc, "sqlite3_open_v2('{}')", _path) );
			try{
				//exec succeeds even when fks are compiled out (the pragma no-ops) - read the setting back instead of trusting rc.
				ExecuteStatement( *_db, "pragma foreign_keys=on", {}, nullptr, sl );
				THROW_IFSL( ScalarUInt(*_db, "pragma foreign_keys", {}, sl).value_or(0)!=1, "Could not enable foreign_keys on '{}' - fks would go unenforced.", _path );
				if( _path!=":memory:" ){
					string mode; //journal_mode=wal reports the resulting mode - stays on the prior journal if wal can't be used (e.g. network fs).
					RowΛ f = [&mode]( Row&& r ){ mode = r.GetString(0); };
					ExecuteStatement( *_db, "pragma journal_mode=wal", {}, &f, sl );
					if( mode!="wal" )
						WARN( "('{}') journal_mode=wal not applied - using '{}'.", _path, mode );
				}
			}
			catch( ... ){ //don't cache a half-configured connection - a retry would skip the pragmas.
				sqlite3_close_v2( _db );
				_db = nullptr;
				throw;
			}
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
		auto sql = insert.Move();
		//Plain inserts: last_insert_rowid covers it - no out param needed, unlike MySql.
		if( !sql.IsProc )
			return Execute( move(sql), sl, {.Sequence=true} );
		//Proc twins return the sequence as their out row - the native equivalent of the generated mysql proc's OUT
		//param - so capture that rather than last_insert_rowid: a multi-statement twin's *last* insert needn't be the
		//sequence table (app_instance_insert inserts app_hosts first).  Execute's Sequence path can't see it: it
		//returns ExecuteProc's rows-affected before reaching the last_insert_rowid line.
		optional<uint> sequence;
		RowΛ f = [&sequence]( Row&& r ){
			if( !sequence && r.Size() )
				sequence = r.GetUInt( 0 );
		};
		let procName = string{ Str::RTrim(sv{sql.Text}.substr(0, sql.Text.find('('))) }; //owned - sql is moved below.
		Execute( move(sql), sl, {.Function=&f} );
		THROW_IFSL( !sequence, "Proc '{}' returned no out row - its twin must emit the sequence column.", procName );
		return *sequence;
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
