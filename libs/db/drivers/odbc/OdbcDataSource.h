#pragma once
#include "Exports.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Syntax.h>

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
		α Select( Sql&& s, RowΛ f, bool outParams, SRCE )ε->uint;
		α Select( Sql&& s, RowΛ f, SRCE )ε->uint override;
		α Select( Sql&& s, SRCE )ε->vector<Row> override;
		α ExecuteSync( Sql&& sql, SRCE )ε->uint override;
		α ExecuteScalerSync( Sql&& sql, EValue outValue, SRCE )ε->DB::Value override;
		α ExecuteNoLog( Sql&& sql, SRCE )ε->uint override;
		α InsertSeqSyncUInt( InsertClause&& insert, SRCE )ε->uint;
		α Query( Sql&& sql, bool outParams, SRCE )ε->QueryAwait override;

		α Syntax()ι->const DB::Syntax& override{ return Syntax::Instance(); }
		α Disconnect()ε->void override;
		α ServerMeta()ι->IServerMeta& override;
		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;
		α SetConfig( const jobject& config )ε->void override;
		α SetConnectionString( string x )ι->void;
	private:
		struct Params final{
			α HasOut()Ι->bool{ return OutValue!=EValue::Null; }
			RowΛ* Function{};
			EValue OutValue{EValue::Null};
			bool Log{true};
			bool Sequence{};
		};
		α ExecDirect( Sql&& sql, SL sl, Params&& params )Ε->uint;
		up<MsSql::MsSqlSchemaProc> _schemaProc;
		string _connectionString;
	};
}