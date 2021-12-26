#include "../../../../Framework/source/db/Syntax.h"

namespace Jde::DB::Sqlite
{
	struct Syntax final: Syntax
	{
		α AddDefault( sv tableName, sv columnName, sv columnDefault )$->string override{ return format("ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, columnDefault); }
		α AltDelimiter()$->sv override{ return "$$"sv; }
		α DateTimeSelect( sv columnName )$->string override{ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }
		α HasUnsigned()$->bool override{ return true; }
		α IdentityColumnSyntax()$->sv override{ return "AUTO_INCREMENT"sv; }
		α IdentitySelect()$->sv override{ return "LAST_INSERT_ID()"sv; }
		β Limit( str sql, uint limit )$->string override{ return format("{} limit {}", sql, limit); }
		α ProcEnd()$->sv override{ return "end"sv; }
		α ProcParameterPrefix()$->sv override{ return ""sv; }
		α ProcStart()$->sv override{ return "begin"sv; }
		α SpecifyIndexCluster()$->bool override{ return false; }
		α UtcNow()$->sv override{ return "CURRENT_TIMESTAMP()"sv; }
		α NowDefault()$->sv override{ return "CURRENT_TIMESTAMP"sv; }
		α ZeroSequenceMode()$->sv override{ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
		α CatalogSelect()$->sv override{ return ".database"; }
		α ProcFileSuffix()$->sv override{ return ""; }
	};
}