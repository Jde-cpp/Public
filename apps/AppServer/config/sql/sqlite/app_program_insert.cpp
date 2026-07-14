#include "appProcs.h"

#define let const auto

//Twin of ../mysql/app_program_insert.sql.
//	params: [0]=_name; out _programId returned as the result row.
namespace Jde::DB::Sqlite::AppProcs{
	α ProgramInsert( IProcs& procs, sqlite3& db, const Value& name, SL sl )ε->uint{
		procs.ExecuteStatement( db, "insert into app_programs( name ) values( ? )", {name}, nullptr, sl );
		return procs.LastInsertRowId( db );
	}

	α RegisterAppProgramInsert( IProcs& procs )ι->void{
		procs.RegisterProc( "app_program_insert", [&procs]( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let programId = ProgramInsert( procs, db, params[0], sl );
			if( onRow )
				(*onRow)( Row{ {Value{programId}} } ); //out _programId
			return 1;
		});
	}
}
