#pragma once
#include <jde/db/Value.h>
#include <jde/db/generators/Sql.h>

namespace Jde::DB::MySql{
	α ToField( const Value& v, SL sl )ε->mysql::field_view;

	//The statement-side half of what the sync Execute and the async MySqlQueryAwait::Main share (db-refactor A7); the
	//result-side half is ToResult in MySqlRow.h.  The connection calls between them are the sync and async spellings of
	//the same boost.mysql api and stay where they are.
	//Every param as a field_view, in order.  A proc's OUT param is its trailing placeholder (IDataSource::ExecuteScalerSync):
	//the caller appends a value for it and the server ignores that value, so it binds like the rest - but with no params
	//at all there is no placeholder to be the OUT param, and that is wrong_num_params before anything reaches the server
	//(#42/#47: the sync path once wrapped `0 - 1` to SIZE_MAX here and walked off the vector).
	α ToFields( const Sql& sql, bool outParam, SL sl )ε->vector<mysql::field_view>;
	//`call p(...)`: the generators emit the bare `p(...)` and each driver adds its own call syntax.
	α CallText( Sql& sql )ι->void;
	//Set on every connection, pooled or one-shot: the driver's datetime convention is UTC (ToField's Time case), and
	//CURRENT_TIMESTAMP/UNIX_TIMESTAMP read the *session* zone, so without this the write half and the read half disagree
	//by the server's offset - and are ambiguous outright across a DST fold.
	constexpr const char* UtcSession{ "set time_zone='+00:00'" };
}