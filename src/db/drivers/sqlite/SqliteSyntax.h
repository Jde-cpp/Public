#include "../../../../Framework/source/db/Syntax.h"

#define $(x) const noexcept->x override
namespace Jde::DB::Sqlite
{
	struct Syntax final: DB::Syntax
	{
		α AddDefault( sv tableName, sv columnName, sv columnDefault )$(string){ return format("ALTER TABLE {} ALTER COLUMN {} SET DEFAULT {}", tableName, columnName, columnDefault); }
		α AltDelimiter()$(sv){ return "$$"sv; }
		α DateTimeSelect( sv columnName )$(string){ return format( "UNIX_TIMESTAMP(CONVERT_TZ({}, '+00:00', @@session.time_zone))", columnName ); }
		α HasUnsigned()$(bool){ return true; }
		α IdentityColumnSyntax()$(sv){ return "AUTO_INCREMENT"sv; }
		α IdentitySelect()$(sv){ return "LAST_INSERT_ID()"sv; }
		β Limit( str sql, uint limit )$(string){ return format("{} limit {}", sql, limit); }
		α ProcEnd()$(sv){ return "end"sv; }
		α ProcParameterPrefix()$(sv){ return ""sv; }
		α ProcStart()$(sv){ return "begin"sv; }
		α SpecifyIndexCluster()$(bool){ return false; }
		α UtcNow()$(sv){ return "CURRENT_TIMESTAMP()"sv; }
		α NowDefault()$(sv){ return "CURRENT_TIMESTAMP"sv; }
		α ZeroSequenceMode()$(sv){ return "SET @@session.sql_mode = CASE WHEN @@session.sql_mode NOT LIKE '%NO_AUTO_VALUE_ON_ZERO%' THEN CASE WHEN LENGTH(@@session.sql_mode)>0 THEN CONCAT_WS(',',@@session.sql_mode,'NO_AUTO_VALUE_ON_ZERO') ELSE 'NO_AUTO_VALUE_ON_ZERO' END ELSE @@session.sql_mode END"sv; }
		α CatalogSelect()$(sv){ return ".database"; }
		α ProcFileSuffix()$(sv){ return ""; }
	};
}
#undef $