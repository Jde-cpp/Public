#include <sqlite3.h>
#include "appProcs.h"

#define let const auto

//Twin of ../mysql/app_program_insert.sql.
//	params: [0]=_name; out _programId returned as the result row.
namespace Jde::DB::Sqlite::AppProcs{
	α ProgramInsert( sqlite3& db, const Value& name, SL sl )ε->uint{
		ExecuteStatement( db, "insert into app_programs( name ) values( ? )", {name}, nullptr, sl );
		return (uint)sqlite3_last_insert_rowid( &db );
	}

	α RegisterAppProgramInsert()ι->void{
		RegisterProc( "app_program_insert", []( sqlite3& db, const vector<Value>& params, RowΛ* onRow, SL sl )->uint{
			let programId = ProgramInsert( db, params[0], sl );
			if( onRow )
				(*onRow)( Row{ {Value{programId}} } ); //out _programId
			return 1;
		});
	}
}
