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
	struct OdbcDataSource : IDataSource{
		α Select( Sql&& s, RowΛ f, bool outParams, SRCE )ε->uint;
		α InsertSeqSyncUInt( InsertClause&& insert, SRCE )ε->uint override;
		α Query( Sql&& sql, bool outParams, SRCE )ε->QueryAwait override;

		α Syntax()ι->const DB::Syntax& override{ return Syntax::Instance(); }
		α Disconnect()ε->void override;
		α ServerMeta()ι->IServerMeta& override;
		α AtCatalog( sv catalog, SRCE )ε->sp<IDataSource> override;
		α AtSchema( sv schema, SRCE )ε->sp<IDataSource> override;
		α SetConfig( const jobject& config )ε->void override;
		α SetConnectionString( string x )ι->void;
	private:
		α Execute( Sql&& sql, SL sl, Params params )ε->uint override; //C1: the one primitive; IDataSource implements the sync wrappers over it.
		up<MsSql::MsSqlSchemaProc> _schemaProc;
		string _connectionString;
	};
}