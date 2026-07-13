#include <sqlite3.h>
#include "SqliteProcs.h"
#include "SqliteRow.h" //Bind/ToRow
#include <jde/db/DBException.h>

#define let const auto

namespace Jde::DB::Sqlite{
	flat_map<string,ProcΛ> _procs; std::shared_mutex _procsMutex;

	α RegisterProc( string name, ProcΛ proc )ι->void{
		ul _{ _procsMutex };
		//ASSERT( !_procs.contains(name) ); TODO
		_procs[move(name)] = move(proc);
	}
	α UnregisterProcs( const vector<string>& names )ι->void{
		ul _{ _procsMutex };
		for( let& name : names )
			_procs.erase( name );
	}
	α FindProc( sv name )ι->const ProcΛ*{
		sl _{ _procsMutex };
		let p = _procs.find( string{name} );
		return p==_procs.end() ? nullptr : &p->second;
	}
	α RegisteredProcNames()ι->vector<string>{
		sl _{ _procsMutex };
		vector<string> names; names.reserve( _procs.size() );
		for( let& [name, _] : _procs )
			names.push_back( name );
		return names;
	}

	α ExecuteStatement( sqlite3& db, sv sql, const vector<Value>& params, RowΛ* onRow, SL sl )ε->uint{
		sqlite3_stmt* stmt{};
		THROW_IFSL( sqlite3_prepare_v2(&db, sql.data(), (int)sql.size(), &stmt, nullptr)!=SQLITE_OK, "prepare failed: {} - {}", sqlite3_errmsg(&db), sql );
		std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)> cleanup{ stmt, &sqlite3_finalize };
		Bind( *stmt, params, sl );
		int rc;
		while( (rc=sqlite3_step(stmt))==SQLITE_ROW ){
			if( onRow )
				(*onRow)( ToRow(*stmt) );
		}
		THROW_IFSL( rc!=SQLITE_DONE, "step failed: {} - {}", sqlite3_errmsg(&db), sql );
		return (uint)sqlite3_changes( &db );
	}

	α ScalarUInt( sqlite3& db, sv sql, const vector<Value>& params, SL sl )ε->optional<uint>{
		optional<uint> y;
		RowΛ f = [&y]( Row&& row ){ if( !row.IsNull(0) ) y = row.GetUInt(0); };
		ExecuteStatement( db, sql, params, &f, sl );
		return y;
	}
}