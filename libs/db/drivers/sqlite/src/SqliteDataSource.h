#pragma once
#include "exports.h"
#include "jde/db/sqlite_api.h"
#include "usings.h"
#include <jde/db/Value.h>
#include <jde/db/IDataSource.h>
#include "SqliteProcs.h"
#include "SqliteSyntax.h"

struct sqlite3;

extern "C" ΓLITE Jde::DB::IDataSource* GetDataSource();

namespace Jde::DB::Sqlite{
	struct SqliteServerMeta;
	class SqliteApi;
	struct SqliteDataSource final : IDataSource{
		~SqliteDataSource() override;
		α ExecuteSync( Sql&& sql, SL sl )ε->uint override;
		α ExecuteScalerSync( Sql&& sql, EValue outValue, SL sl )ε->Value override;
		α ExecuteNoLog( Sql&& sql, SRCE )ε->uint override;
		α Select( Sql&& s, SRCE )ε->vector<Row> override;
		α Select( Sql&& s, RowΛ f, SRCE )ε->uint override; //public - SqliteQueryAwait executes through it.
		α Query( Sql&& sql, bool outParams, SRCE )ε->QueryAwait override;

		α ServerMeta()ι->IServerMeta& override;
		α Syntax()ι->const DB::Syntax& override{ return SqliteSyntax::Instance(); }

		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override; //no catalogs - returns self.
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;   //'main' only; ATTACH could emulate others.
		α SchemaNameConfig( SL=SRCE_CUR )ι->string override{ return "main"; }
		α SetConfig( const jobject& config )ε->void override;
		α Disconnect()ε->void override;
	private:
		struct Params final{
			α HasOut()Ι->bool{ return OutValue!=EValue::Null; }
			RowΛ* Function{};
			EValue OutValue{EValue::Null};
			bool Log{true};
			bool Sequence{false};
		};
		α Execute( DB::Sql&& sql, SL sl, Params exeParams )ε->uint;
		α Execute( DB::Sql&& sql, SL sl )ε->uint{ return Execute( move(sql), sl, {} ); }
		α InsertSeqSyncUInt( DB::InsertClause&& insert, SL sl )ε->uint override;
		α Connection( SL sl )ε->sqlite3&; //lazy open.
		α ExecuteProc( DB::Sql& sql, SL sl, Params& exeParams )ε->uint; //dispatch to SqliteProcs registry inside a transaction.

		flat_map<fs::path,up<SqliteApi>> _procDlls;
		//Single connection, serialized by _connMutex: an in-memory db is per-connection, so a MySql-style
		//session pool would hand each caller its own empty database. For file-backed dbs a pool + WAL is an option.
		sqlite3* _db{};
		std::mutex _connMutex;
		string _path{ ":memory:" };
		up<SqliteServerMeta> _serverMeta;
	};
}
