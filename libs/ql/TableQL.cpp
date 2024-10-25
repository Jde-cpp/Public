#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/FilterQL.h>
#include <jde/db/names.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
#include <jde/db/meta/Column.h>
#include <jde/db/generators/Syntax.h>

#define let const auto
namespace Jde::QL{
	using DB::EOperator;
	α TableQL::DBName()Ι->string{
		return DB::Names::ToPlural( DB::Names::FromJson(JsonName) );
	}
	α TableQL::Filter()Ε->FilterQL{
		FilterQL filters;
		let filterArgs = Json::FindObject( Args, "filter" );
		let& j = filterArgs ? Args : *filterArgs;
		for( let& [jsonColumnName,value] : j ){
			vector<FilterValueQL> columnFilters;
			if( value.is_string() || value.is_number() || value.is_null() ) //(id: 42) or (name: "charlie") or (deleted: null)
				columnFilters.emplace_back( DB::EOperator::Equal, value );
			else if( value.is_object() ){ //filter: {age: {gt: 18, lt: 60}}
				for( let& [op,opValue] : value.as_object() )
					columnFilters.emplace_back( ToQLOperator(op) );
			}
			else if( value.is_array() ) //(id: [1,2,3]) or (name: ["charlie","bob"])
				columnFilters.emplace_back( EOperator::In, value );
			else
				THROW("Invalid filter value type '{}'.", Json::Kind(value.kind()) );
			filters.ColumnFilters.emplace( jsonColumnName, columnFilters );
		}
		return filters;
	}

	α ColumnQL::QLType( const DB::Column& column, SL sl )ε->string{
		string qlTypeName = "ID";
		if( !column.SKIndex ){
			switch( column.Type ){ using enum DB::EType;
			case Bit:
				qlTypeName = "Boolean";
				break;
			case Int16: case Int: case Int8: case Long:
				qlTypeName = "Int";
				break;
			case UInt16: case UInt: case ULong:
				qlTypeName = "UInt";
				break;
			case SmallFloat: case Float: case Decimal: case Numeric: case Money:
				qlTypeName = "Float";
				break;
			case None: case Binary: case VarBinary: case Guid: case Cursor: case RefCursor: case Image: case Blob: case TimeSpan:
				throw Exception{ sl, ELogLevel::Debug, "EType {} is not implemented.", (uint)column.Type };
			case VarWChar: case VarChar: case NText: case Text: case Uri:
				qlTypeName = "String";
				break;
			case WChar: case UInt8: case Char:
				qlTypeName = "Char";
			case DateTime: case SmallDateTime:
				qlTypeName = "DateTime";
				break;
			}
		}
		return qlTypeName;
	}
}