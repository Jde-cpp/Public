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
		α Execute( Sql&& sql, bool prepare, SL sl )ε->uint override;
		α ExecuteNoLog( Sql&& sql, RowΛ* f=nullptr, SRCE )ε->uint override;
		//α ExecuteAsync( Sql&& sql, SRCE )ε->ExecuteAwait override;
		α Select( Sql&& s, SRCE )ε->vector<Row> override;
		//α SelectAsync( Sql&& sql, SRCE )ι->SelectAwait override;
		α Query( Sql&& sql, SRCE )ε->QueryAwait override;

		α ServerMeta()ι->IServerMeta& override;
		α Syntax()ι->const DB::Syntax& override{ return MySqlSyntax::Instance(); }

		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;
		α SchemaNameConfig( SRCE )ι->string override;
		α SetConfig( const jobject& config )ε->void;
		α Disconnect()ε->void override{ THROW("Not implemented"); }
		α ConnectionParams()ι->const mysql::connect_params&{ return _cs; }
	private:
		α Execute( DB::Sql&& sql, const RowΛ* f, bool prepare, SL sl, bool log )ε->uint;
		α Select( Sql&& s, RowΛ f, SL sl )ε->uint override;
		up<MySqlServerMeta> _schemaProc;
		mysql::connect_params _cs;
	};
}