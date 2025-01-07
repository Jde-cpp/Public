#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/FilterQL.h>
#include <jde/db/IRow.h>
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
	α TableQL::AddFilter( const string& column, const jvalue& value )ι->void{
		auto destination = &Args;
		if( auto filter = Args.find( "filter" ); filter!=Args.end() )
			destination = &filter->value().as_object();
		(*destination)[column] = value;
	}

	α TableQL::FindTable( sv jsonPluralName )ι->TableQL*{
		TableQL* y{};
		if( auto p = find_if( Tables, [&](let& t){return t.JsonName==jsonPluralName;}); p!=Tables.end() )
			y = &*p;
		else if( auto p = find_if( Tables.begin(), Tables.end(), [&](let& t){return t.JsonName==DB::Names::ToSingular(jsonPluralName);}); p!=Tables.end() )
			y = &*p;
		return y;
	}
	α TableQL::FindTable( sv jsonPluralName )Ι->const TableQL*{
		return const_cast<TableQL*>(this)->FindTable( jsonPluralName );
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
	α TableQL::ToJson( DB::IRow& row, const vector<sp<DB::Column>>& dbColumns )Ι->jobject{
		jobject y;
		for( uint i=0; i<dbColumns.size() && i<row.Size(); ++i )
			SetResult( y, dbColumns[i], move(row[i]) );
		return y;
	}
	α ValueToJson( DB::Value&& dbValue, const ColumnQL* pMember=nullptr )ι->jvalue;
	α TableQL::SetResult( jobject& o, const sp<DB::Column> dbColumn, DB::Value&& value )Ι->void{
		for( let& c : Columns ){
			if( c.DBColumn==dbColumn ){
				o[dbColumn->IsPK() && !dbColumn->IsEnum() ? "id" : c.JsonName] = ValueToJson( move(value), &c );
				return;
			}
		}
		for( let& t : Tables ){
			if( !o.contains(t.JsonName) )
				o[t.JsonName] = jobject{};
			t.SetResult( o.at(t.JsonName).as_object(), dbColumn, move(value) );
		}
	}
	α TableQL::ToString()Ι->string{
		string y = JsonName;
		y.resize( 64*(1+Tables.size()) );
		if( Args.size() ){
			auto args = serialize( Args );
			args.front() = '(';
			args.back() = ')';
			y += move( args );
		}
		y += '{';
		if( Columns.size() ){
			vector<string> cols;
			for_each( Columns, [&cols](let& c){cols.push_back(c.JsonName);} );
			y += Str::Join( cols, " " );
		}
		if( Tables.size() ){
			for( let& t : Tables ){
				if( t.Args.size() || t.Columns.size() || t.Tables.size() )
					y += ' '+t.ToString();
			}
		}
		y += '}';
		return y;
	}
	α TableQL::TrimColumns( const jobject& fullOutput )Ι->jobject{
		jobject y;
		for( let& c : Columns ){
			if( auto p = fullOutput.if_contains(c.JsonName); p )
				y[c.JsonName] = *p;
		}
		for( let& t : Tables ){
			if( auto p = fullOutput.if_contains(t.JsonName); p )
				y[t.JsonName] = t.TrimColumns( Json::AsObject(*p) );
		}
		return y;
	}
}