#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/FilterQL.h>
#include <jde/db/Row.h>
#include <jde/db/Key.h>
#include <jde/db/names.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/DBSchema.h>
#include <jde/db/meta/Column.h>

#define let const auto
namespace Jde::QL{
	using DB::EOperator;

	α dbTable( string jName, const vector<sp<DB::AppSchema>>& schemas, bool system, SL sl )ε->sp<DB::View>{
		let dbName = DB::Names::ToPlural( DB::Names::FromJson(move(jName)) );
		return system ? DB::AppSchema::FindView( schemas, dbName ) : DB::AppSchema::GetViewPtr( schemas, dbName, sl );
	}

	TableQL::TableQL( string jName, jobject args, sp<jobject> variables, const vector<sp<DB::AppSchema>>& schemas, bool system, SL sl )ε:
		Input{ move(args), move(variables) },
		JsonName{ jName },
		_dbTable{ dbTable(jName, schemas, system, sl) }
	{}

	α addFilters( const jvalue& value, const sp<jobject>& variables )ε->vector<FilterValue>{
			vector<FilterValue> columnFilters;
			if( value.is_string() || value.is_number() || value.is_null() ) //( id: 42 ) or ( name: "charlie" ) or ( deleted: null )
				columnFilters.emplace_back( DB::EOperator::Equal, value );
			else if( value.is_object() ){ //filter: {age: {gt: 18, lt: 60}}
				for( let& [op,opValue] : value.as_object() ){
					if( opValue.is_string() && opValue.get_string().starts_with("\b$") ){
						if( auto p = variables->if_contains(sv{opValue.get_string()}.substr(2)); p ){
							columnFilters.emplace_back( ToQLOperator(op), *p );
							continue;
						}
					}
					columnFilters.emplace_back( ToQLOperator(op), opValue );
				}
			}
			else if( value.is_array() ) //( id: [1,2,3] ) or ( name: ["charlie","bob"] )
				columnFilters.emplace_back( EOperator::In, value );
			else
				THROW( "Invalid filter value type '{}'.", Json::Kind(value.kind()) );
			return columnFilters;
	}
	α TableQL::Filter()Ε->const QL::Filter&{
		if( _filter )
			return *_filter;
		//let filterArgs = Json::FindObject( Args, "filter" );
		//let& j = filterArgs ? Args : *filterArgs;
		_filter = QL::Filter{};
		for( let& [jsonColumnName,value] : Args ){
			jvalue* variable = nullptr;
			if( value.is_string() && value.get_string().starts_with("\b$") ){
				if( auto p = Variables->if_contains(sv{value.get_string()}.substr(2)); p )
					variable = p;
			}
			_filter->ColumnFilters.emplace( jsonColumnName, addFilters(variable ? *variable : value, Variables) );
		}
		return *_filter;
	}
	α TableQL::AddColumn( sv jsonName )ι->bool{
		auto existing = FindColumn( jsonName );
		if( existing )
			return false;

		ASSERT( _dbTable );
		auto dbColumn = _dbTable->FindColumn( DB::Names::FromJson(jsonName) );
		Columns.push_back( ColumnQL{string{jsonName}, dbColumn} );
		return true;
	}

	α TableQL::AddFilter( const string& column, const jvalue& value )ι->void{
		auto destination = &Args;
		if( auto filter = Args.find("filter"); filter!=Args.end() )
			destination = &filter->value().as_object();
		( *destination )[column] = value;
	}

	α TableQL::ExtractTable( sv jsonPluralName )ι->optional<TableQL>{
		auto p = find_if( Tables, [&](let& t){return t.JsonName==jsonPluralName;} );
		if( p==Tables.end() )
			p = find_if( Tables.begin(), Tables.end(), [&](let& t){return t.JsonName==DB::Names::ToSingular(jsonPluralName);} );
		if( p==Tables.end() )
			return {};
		auto y = move( *p );
		Tables.erase( p );
		return y;
	}

	α TableQL::FindTable( sv jsonPluralName )ι->TableQL*{
		TableQL* y{};
		if( auto p = find_if(Tables, [&](let& t){return t.JsonName==jsonPluralName;}); p!=Tables.end() )
			y = &*p;
		else if( auto p = find_if(Tables.begin(), Tables.end(), [&](let& t){return t.JsonName==DB::Names::ToSingular(jsonPluralName);}); p!=Tables.end() )
			y = &*p;
		return y;
	}
	α TableQL::FindTable( sv jsonPluralName )Ι->const TableQL*{
		return const_cast<TableQL*>( this )->FindTable( jsonPluralName );
	}

	α TableQL::GetTable( sv jsonPluralName, SL sl )ε->TableQL&{
		TableQL* y = FindTable( jsonPluralName );
		THROW_IFSL( !y, "Could not find table '{}'.", jsonPluralName );
		return *y;
	}

	α TableQL::FindArgKey()Ι->optional<DB::Key>{
		optional<DB::Key> y;
		if( let id = Json::FindValue(Args, "id"); id )
			y = DB::Key{ Json::AsNumber<uint>(move(*id)) };
		else if( let target = Json::FindValue(Args, "target"); target )
			y = DB::Key{ Json::AsString(move(*target)) };
		return y;
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
				break;
			case DateTime: case SmallDateTime:
				qlTypeName = "DateTime";
				break;
			}
		}
		return qlTypeName;
	}
	α TableQL::ToJson( DB::Row& row, const vector<DB::Object>& dbColumns )Ι->jobject{
		jobject y;
		for( uint i=0; i<dbColumns.size() && i<row.Size(); ++i ){
			let col = std::get_if<DB::AliasCol>( &dbColumns[i] );
			if( col )
				SetResult( y, col->Column, move(row[i]) );
			else
				CRITICALT( ELogTags::QL, "Column {} is not an AliasCol.", i );
		}
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
	α TableQL::TransformResult( jarray&& result )Ι->jvalue{
		jvalue v;
		if( IsPlural() )
			v = move( result );
		else
			v = result.size() ? move( result[0].as_object() ) : jobject{};
		return ReturnRaw ? move( v ) : jobject{ {ReturnName(), move(v)} };
	}
	α TableQL::TransformResult( jobject&& result )Ι->jobject{
		return ReturnRaw ? move( result ) : jobject{ {ReturnName(), move(result)} };
	}
	α TableQL::TransformResult( string&& result )Ι->jvalue{
		jvalue v{ move(result) };
		return ReturnRaw ? move( v ) : jobject{ {ReturnName(), move(v)} };
	}
	α TableQL::ToString()Ι->string{
		string y = JsonName;
		y.reserve( 64*(1+Tables.size()) );
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