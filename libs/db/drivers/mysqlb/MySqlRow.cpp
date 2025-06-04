#include "MySqlRow.h"
#include <jde/db/Value.h>
#include <jde/db/DBException.h>
#include "../../../../../Framework/source/math/MathUtilities.h"
#define let const auto

namespace Jde::DB{
	using namespace std::chrono;

/*	α GetTypeName( mysqlx::Value mySqlValue )->string{
		string value;
		switch( mySqlValue.getType() ){
			using enum mysqlx::Value::Type;
		case VNULL: value = "null"; break;
		case UINT64: value = "uint"; break;
		case INT64: value = "int"; break;
		case FLOAT: value = "float"; break;
		case DOUBLE: value = "double"; break;
		case BOOL: value = "bool"; break;
		case STRING: value = "string"; break;
		case DOCUMENT: value = "document"; break;
		case RAW: value = "raw"; break;
		case ARRAY: value = "array"; break;
		default: value = "unknown"; break;
		};
		return value;
	}
*/

	Ω toValue( const boost::mysql::field_view& field, SL sl )ε->Value{
		Value v;
		switch( field.kind() ){
			using enum boost::mysql::field_kind;
    	case null: break;
			case int64: v = Value{ field.as_int64() }; break;
			case uint64: v = Value{ field.as_uint64() }; break;
			case string: v = Value{ field.as_string() }; break;
			case float_: v = Value{ field.as_float() }; break;
			case double_: v = Value{ field.as_double() }; break;
			case date:{
				std::chrono::time_point<std::chrono::system_clock, std::chrono::days> tp{ field.as_date().get_time_point() };
				DBTimePoint time{ duration_cast<DBTimePoint::duration>( tp.time_since_epoch() ) };
				v = Value{ time };
			}break;
			case datetime:{
				boost::mysql::datetime::time_point dt{ field.as_datetime().get_time_point() };
				v = Value{ DBTimePoint{duration_cast<DBTimePoint::duration>(dt.time_since_epoch())} };
			}break;
			case time:{
				std::chrono::microseconds micros{ field.as_time() };
				v = Value{ micros.count() };
			}break;
			case blob:
				v = Value{ vector<uint8_t>{field.as_blob().begin(), field.as_blob().end()} }; break;
		}
		return v;
	}

	α MySql::ToRow( mysql::row_view& mySqlRow, SL sl )->Row{
		vector<Value> values;
		for( auto&& field : mySqlRow )
			values.push_back( toValue(field, sl) );
		return Row{ move(values) };
	}
/*
	MySqlRow::MySqlRow( boost::mysql::row_view&& row, SL sl )ε:
		_values{ getFields(row, sl) }
	{}

	α MySqlRow::Move()ι->up<IRow>{
		return mu<MySqlRow>( move(*this) );
	}

	α MySqlRow::operator[]( uint i )Ι->const Value&{
		return _values[i];
	}
	α MySqlRow::operator[]( uint i )ι->Value&{
		return _values[i];
	}

	_int MySqlRow::GetInt( uint i, SL sl )Ε{
		return _values[i].get_number<_int>();
	}

		optional<_int> MySqlRow::GetIntOpt( uint i, SL sl )Ε{
		let& value = _values[i];
		return value.is_null() ? optional<_int>{} : GetInt( i, sl );
	}

	uint MySqlRow::GetUInt( uint i, SL sl )Ε{
		return _values[i].get_number<uint>();
	}

	bool MySqlRow::GetBit( uint i, SL sl )Ε{return GetInt( i )!=0;}

	optional<uint> MySqlRow::GetUIntOpt( uint i, SL sl )Ε{
		let& value = _values[i];
		return value.is_null() ? optional<uint>{} : GetUInt( i );
	}

	α MySqlRow::MoveString( uint i, SL sl )ε->string{
		auto& value = _values[i];
		return value.is_null() ? string{} : move(value.get_string());
	}
	α MySqlRow::GetString( uint i, SL sl )Ε->string{
		auto& value = _values[i];
		return value.is_null() ? string{} : value.get_string();
	}

	double MySqlRow::GetDouble( uint i, SL sl )Ε{
		return _values[i].get_double();
	}

	α MySqlRow::GetDoubleOpt( uint i, SL sl )Ε->optional<double>{
		let& value = _values[i];
		return value.is_null() ? optional<double>{} : GetDouble(i);
	}

	α MySqlRow::GetTimePoint( uint i, SL sl )Ε->DBTimePoint{
		DBTimePoint y;
		let& value = _values[i];
		if( value.is_double() ){
			let doubleValue = GetDouble( i );
			y = DBClock::from_time_t( (int)doubleValue );
			y+=microseconds( Round((doubleValue-(uint)doubleValue)*1'000'000) );
		}
		else if( value.is_int32() )
			y = DBClock::from_time_t( GetInt(i) );
		else
			throw Exception( sl, "TimePoint not implemented for type '{}'", value.TypeName() );

		return y;
	}

	α MySqlRow::GetTimePointOpt( uint i, SL sl )Ε->optional<DBTimePoint>{
		let& value = GetIntOpt( i );
		return value.has_value() ? DBClock::from_time_t(*value) : optional<DBTimePoint>{};
	}
*/
}