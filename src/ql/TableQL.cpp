#include <jde/ql/TableQL.h>
#include <jde/ql/FilterQL.h>
#include <jde/db/metadata/Schema.h>

#define var const auto
namespace Jde::QL{

	α TableQL::DBName()Ι->string{
		return DB::Schema::ToPlural( DB::Schema::FromJson(JsonName) );
	}
	α TableQL::Filter()Ε->FilterQL{
		FilterQL filters;
		var filterArgs = Args.find( "filter" );
		var j = filterArgs==Args.end() ? Args : *filterArgs;
		for( var& [jsonColumnName,value] : j.items() ){
			vector<FilterValueQL> columnFilters;
			if( value.is_string() || value.is_number() || value.is_null() ) //(id: 42) or (name: "charlie") or (deleted: null)
				columnFilters.emplace_back( EQLOperator::Equal, value );
			else if( value.is_object() ){ //filter: {age: {gt: 18, lt: 60}}
				for( var& [op,opValue] : value.items() )
					columnFilters.emplace_back( Str::ToEnum<EQLOperator>(EQLOperatorStrings, op).value_or(EQLOperator::Equal), opValue );
			}
			else if( value.is_array() ) //(id: [1,2,3]) or (name: ["charlie","bob"])
				columnFilters.emplace_back( EQLOperator::In, value );
			else
				THROW("Invalid filter value type '{}'.", value.type_name() );
			filters.ColumnFilters.emplace( jsonColumnName, columnFilters );
		}
		return filters;
	}

	α MutationQL::TableSuffix()Ι->string{
		if( _tableSuffix.empty() )
			_tableSuffix = DB::Schema::ToPlural<string>( DB::Schema::FromJson<string,string>(JsonName) );
		return _tableSuffix;
	}

	α MutationQL::InputParam( sv name )Ε->json{
		var input = Input();
		var p = input.find( name ); THROW_IF( p==input.end(), "Could not find '{}' argument. {}", name, input.dump() );
		return *p;
	}
	α MutationQL::Input(SL sl)Ε->json{
		var pInput = Args.find("input"); THROW_IFSL( pInput==Args.end(), "Could not find input argument. '{}'", Args.dump() );
		return *pInput;
	}

	α ColumnQL::QLType( const DB::Column& column, SL sl )ε->string{
		string qlTypeName = "ID";
		if( !column.IsId ){
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
			case VarTChar: case VarWChar: case VarChar: case NText: case Text: case Uri:
				qlTypeName = "String";
				break;
			case TChar: case WChar: case UInt8: case Char:
				qlTypeName = "Char";
			case DateTime: case SmallDateTime:
				qlTypeName = "DateTime";
				break;
			}
		}
		return qlTypeName;
	}
}