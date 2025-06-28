#pragma once
#include "exports.h"
#include "usings.h"
#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

extern "C" ΓMY Jde::DB::IDataSource* GetDataSource();

namespace Jde::DB::MySql{
	struct MySqlServerMeta;
	struct MySqlDataSource final : IDataSource{
		α ExecuteSync( Sql&& sql, SL sl )ε->uint override;
		α ExecuteScalerSync( Sql&& sql, EValue outValue, SL sl )ε->Value override;
		α ExecuteNoLog( Sql&& sql, SRCE )ε->uint override;
		α Select( Sql&& s, SRCE )ε->vector<Row> override;
		α Query( Sql&& sql, bool outParams, SRCE )ε->QueryAwait override;

		α ServerMeta()ι->IServerMeta& override;
		α Syntax()ι->const DB::Syntax& override{ return MySqlSyntax::Instance(); }

		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;
		α SchemaNameConfig( SRCE )ι->string override;
		α SetConfig( const jobject& config )ε->void;
		α Disconnect()ε->void override{ THROW("Not implemented"); }
		α ConnectionParams()ι->const mysql::connect_params&{ return _cs; }
	private:
		struct Params final{
			α HasOut()Ι->bool;
			RowΛ* Function{};
			EValue OutValue{EValue::Null};
			bool Log{true};
			bool Sequence{false};
		};
		α Execute( DB::Sql&& sql, SL sl, Params exeParams )ε->uint;
		α Execute( DB::Sql&& sql, SL sl )ε->uint{ return Execute( move(sql), sl, {} ); }
		α Select( Sql&& s, RowΛ f, SL sl )ε->uint override;
		α InsertSeqSyncUInt( DB::InsertClause&& insert, SL sl )ε->uint override;
		up<MySqlServerMeta> _schemaProc;
		mysql::connect_params _cs;
	};
}