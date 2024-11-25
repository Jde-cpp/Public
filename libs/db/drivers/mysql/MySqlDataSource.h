#pragma once
#include "exports.h"
#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

extern "C" ΓMY Jde::DB::IDataSource* GetDataSource();

namespace Jde::DB::MySql{
	struct MySqlServerMeta;
	struct MySqlDataSource final : IDataSource{
		α Execute( string sql, SL sl )ε->uint override;
		α Execute( string sql, const vector<Value>& parameters, SL sl )ε->uint override;
		α Execute( string sql, const vector<Value>* pParameters, const RowΛ* f, bool isStoredProc=false, SRCE )ε->uint override;
		α ExecuteProc( string sql, const vector<Value>& parameters, SL sl )ε->uint override;
		α ExecuteProc( string sql, const vector<Value>& parameters, RowΛ f, SL sl )ε->uint override;
		α ExecuteProcCo( string sql, vector<Value> p, SL sl )ι->up<IAwait> override{ return ExecuteCo( move(sql), move(p), true, sl ); }
		α ExecuteProcCo( string sql, vector<Value> p, RowΛ f, SRCE )ε->up<IAwait>{ return ExecuteCo( move(sql), move(p), true, f, sl ); }
		α ExecuteCo( string sql, vector<Value> p, SRCE )ι->up<IAwait> override{ return ExecuteCo(move(sql), move(p), false, sl); }
		α ExecuteCo( string sql, vector<Value> p, bool proc, SRCE )ι->up<IAwait>;
		α ExecuteCo( string sql, vector<Value> p, bool proc, RowΛ f, SRCE )ε->up<IAwait>;
		β SchemaNameConfig( SRCE )ε->string override;
		α Select( Sql&& s, bool storedProc, SRCE )Ε->vector<up<IRow>> override;
		α SelectCo( ISelect* pAwait, string sql, vector<Value>&& params, SL sl )ι->up<IAwait> override;
		α ServerMeta()ι->IServerMeta& override;
		α Syntax()ι->const DB::Syntax& override{ return MySqlSyntax::Instance(); }

		α ExecuteNoLog( string sql, const vector<Value>* pParameters, RowΛ* f, bool isStoredProc, SL sl )ε->uint override;
		α ExecuteProcNoLog( string sql, vec<Value> parameters, SL sl )ε->uint override;
		α SelectNoLog( string sql, RowΛ f, const vector<Value>* pValues, SL sl )ε->uint override;

		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;

	private:
		α Select( string sql, RowΛ f, const vector<Value>* pValues, SRCE )ε->uint override;
		up<MySqlServerMeta> _pSchemaProc;
	};
}