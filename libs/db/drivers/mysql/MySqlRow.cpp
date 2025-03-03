#include "MySqlRow.h"
#include <jde/db/Value.h>
#include <jde/db/DBException.h>
#include "../../../../../Framework/source/math/MathUtilities.h"
#define let const auto

namespace Jde::DB::MySql{
	using namespace std::chrono;

	α GetTypeName( mysqlx::Value mySqlValue )->string{
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

	Ω toValue( const mysqlx::Value& value, SL sl )ε->Value{
		Value v;
		switch( value.getType() ){
			using enum mysqlx::Value::Type;
			case VNULL: v = Value{ nullptr }; break;
			case STRING: v = Value{ value.get<string>() }; break;
			case BOOL: v = Value{ value.get<bool>() };  break;
			case INT64: v = Value{ static_cast<_int>(value.get<_int>()) };  break;
			case UINT64: v = Value{ value.get<uint>() };  break;
			case DOUBLE: v = Value{ value.get<double>() };  break;
			default: throw Exception{ sl, ELogLevel::Debug, "{} Value not implemented", (uint)value.getType() };
		}
		return v;
	}

	Ω getValues( const mysqlx::Row& row, SL sl )->vector<Value>{
		vector<Value> values;
		for( uint i=0; i<row.colCount(); ++i )
			values.push_back( toValue(row[i], sl) );
		return values;
	}

	MySqlRow::MySqlRow( const mysqlx::Row& row, SL sl )ε:
		_values{ getValues(row, sl) }
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
}