#pragma once
#include "Exports.h"
#include "../../Framework/source/coroutine/Awaitable.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>
#include "OdbcAwaitables.h"
//#include "Binding.h"

extern "C" ΓODBC Jde::DB::IDataSource* GetDataSource();
namespace Jde::DB {
	struct IServerMeta;
	struct Sql;
	namespace Types { struct IRow; }
	namespace MsSql { struct MsSqlSchemaProc; }
}
namespace Jde::DB::Odbc{
	using namespace Coroutine;
	struct OdbcDataSource : IDataSource{
		α Select( Sql&& s, RowΛ f, SRCE )ε->uint override;
		α Select( Sql&& s, SRCE )ε->vector<Row> override;
		α Execute( Sql&& sql, bool prepare=false, SRCE )ε->uint;
		α ExecuteNoLog( Sql&& sql, SRCE )ε->uint override;
		α Query( Sql&& sql, SRCE )ε->QueryAwait override;

		α Syntax()ι->const DB::Syntax& override{ return Syntax::Instance(); }
		α Disconnect()ε->void override;
		α ServerMeta()ι->IServerMeta& override;
		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;
		α SetConfig( const jobject& config )ε->void override;
		α SetConnectionString( string x )ι->void;
	private:
		α ExecDirect( Sql&& sql, const RowΛ* f, bool prepare, SL sl, bool log = true )Ε->uint;
		up<MsSql::MsSqlSchemaProc> _schemaProc;
		string _connectionString;
	};
}