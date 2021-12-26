#include "SqliteSchemaProc.h"


namespace Jde::DB::Sqlite
{
	α SqliteSchemaProc::LoadTables( sv catalog )noexcept(false)->flat_map<string,Table>
	{
		string catalogLocal;
		if( catalog.empty() )
			catalog = catalogLocal = _pDataSource->Catalog( string{DB::MySqlSyntax{}.CatalogSelect()} );
		auto pTables = mu<flat_map<string,Table>>();
		auto result2 = [&]( sv tableName, sv name, _int ordinalPosition, sv dflt, string isNullable, sv type, optional<_int> maxLength, _int isIdentity, _int isId, optional<_int> numericPrecision, optional<_int> numericScale )
		{
			auto& table = pTables->emplace( tableName, Table{catalog,tableName} ).first->second;
			table.Columns.resize( ordinalPosition );

			table.Columns[ordinalPosition-1] = Column{ name, (uint)ordinalPosition, dflt, isNullable!="NO", ToType(type), maxLength.value_or(0), isIdentity!=0, isId!=0, numericPrecision, numericScale };
		};
		auto result = [&]( const IRow& row )
		{
			result2( row.GetString(0), row.GetString(1), row.GetInt(2), row.GetString(3), row.GetString(4), row.GetString(5), row.GetIntOpt(6), row.GetInt(7), row.GetInt(8), row.GetIntOpt(9), row.GetIntOpt(10) );
		};
		var sql = Sql::ColumnSql( false );
		_pDataSource->Select( sql, result, {catalog} );
		return pTables;
	}

}