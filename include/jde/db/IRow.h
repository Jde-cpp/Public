#pragma once
#ifndef ROW_H
#define ROW_H
#include "Value.h"
#include <jde/framework/str.h>

namespace Jde::DB{
/*
	struct IRow{
		//virtual ~IRow(){}
		β operator[]( uint value )Ι->const Value& = 0;
		β operator[]( uint i )ι->Value& = 0;
		Ŧ Get( uint i )Ε->T;
		β GetBit( uint i )Ε->bool=0;
		β GetDouble( uint i )Ε->double=0;
		β GetDoubleOpt( uint i )Ε->optional<double> = 0;
		β GetFloat( uint i )Ε->float{ return static_cast<float>( GetDouble(i) ); }
		β GetGuid( uint i )Ε->boost::uuids::uuid;
		β GetInt( uint i )Ε->int64_t=0;
		β GetInt32( uint i )Ε->int32_t=0;
		β GetIntOpt( uint i )Ε->optional<_int> = 0;
		β GetString( uint i )Ε->string=0;
		β GetTimePoint( uint i )Ε->DBTimePoint=0;
		β GetTimePointOpt( uint i )Ε->optional<DBTimePoint> = 0;
		β GetUInt( uint i )Ε->uint=0;
		β GetUInt32( uint i )Ε->uint32_t{ return static_cast<uint32_t>(GetUInt(i)); }
		β GetUInt16( uint i )Ε->uint16_t{ return static_cast<uint16_t>(GetUInt(i)); }
		β GetUIntOpt( uint i )Ε->optional<uint> = 0;
		β MoveString( uint i )ε->string=0;
		β Size()Ι->uint=0;

		//friend α operator>>( const IRow& row, string& str )ε->const IRow&{ str=row.GetString(row._index++); return row; }
	/ *	friend α operator>>( const IRow& row, uint8_t& value )ε->const IRow&{ value=static_cast<uint8_t>(row.GetUInt(row._index++)); return row; }
		friend α operator>>( const IRow& row, uint64_t& value )ε->const IRow&{ value=row.GetUInt(row._index++); return row; }
		friend α operator>>( const IRow& row, uint32_t& value )ε->const IRow&{ value=static_cast<uint32_t>(row.GetUInt(row._index++)); return row; }
		friend α operator>>( const IRow& row, optional<uint64_t>& value )ε->const IRow&{ value=row.GetUIntOpt(row._index++); return row; }

		friend α operator>>( const IRow& row, long long& value )ε->const IRow&{ value=row.GetInt(row._index++); return row; }
		friend α operator>>( const IRow& row, long& value )ε->const IRow&{ value=static_cast<int32_t>(row.GetInt(row._index++)); return row; }
		friend α operator>>( const IRow& row, optional<long>& value )ε->const IRow&{ auto value2=row.GetIntOpt(row._index++); if( value2.has_value()) value = static_cast<long>(value2.value()); else value=std::nullopt; return row; }
		friend α operator>>( const IRow& row, optional<long long>& value )ε->const IRow&{ value=row.GetIntOpt(row._index++); return row; }

		friend α operator>>( const IRow& row, double& value )ε->const IRow&{ value=row.GetDouble(row._index++); return row; }
		friend α operator>>( const IRow& row, optional<double>& value )ε->const IRow&{ value=row.GetDoubleOpt(row._index++); return row; }

		friend α operator>>( const IRow& row, float& value )ε->const IRow&{ value=static_cast<float>(row.GetDouble(row._index++)); return row; }
		friend α operator>>( const IRow& row, optional<float>& value )ε->const IRow&{ auto dValue = row.GetDoubleOpt(row._index++); if( dValue.has_value() ) value=static_cast<float>(dValue.value()); else value=std::nullopt; return row; }

		friend α operator>>( const IRow& row, DBTimePoint& value)ε->const IRow&{ value=row.GetTimePoint(row._index++); return row; }
		friend α operator>>( const IRow& row, optional<DBTimePoint>& value)ε->const IRow&{ value=row.GetTimePointOpt(row._index++); return row; }
* /
//	protected:
//		mutable uint _index{0};
	};

	template<> Ξ IRow::Get<bool>( uint i )Ε->bool{ return GetBit(i); }
	template<> Ξ IRow::Get<string>( uint i )Ε->string{ return GetString(i); }
	template<> Ξ IRow::Get<uint>( uint i )Ε->uint{ return GetUInt(i); }
	template<> Ξ IRow::Get<unsigned int>( uint i )Ε->unsigned int{ return (unsigned int)GetUInt(i); }
	template<> Ξ IRow::Get<optional<uint32>>( uint i )Ε->optional<uint32>{ const auto p = GetUIntOpt(i); return p ? optional<uint32>{static_cast<uint32>(*p)} : optional<uint32>{}; }
	template<> Ξ IRow::Get<uint8>( uint i )Ε->uint8{ return (uint8)GetUInt16(i); }
	template<> Ξ IRow::Get<long>( uint i )Ε->long{ return (long)GetInt32(i); }
	template<> Ξ IRow::Get<optional<TimePoint>>( uint i )Ε->optional<TimePoint>{ return GetTimePointOpt(i); }
	Ŧ IRow::Get( uint i )Ε->T{ return T{Get<typename T::Type>(i)}; }//for pk
*/
	struct Row final/*: IRow*/ {
		Row( vector<Value>&& values )ι:_values{move(values)}{}
		α operator[]( uint i )Ι->const Value&{ return _values[i]; }
		α operator[]( uint i )ι->Value&{ return _values[i]; }
		Ŧ Get( uint i )Ι->T;
		Ŧ GetOpt( uint i )Ι->optional<T>;
		α GetBit( uint i )Ι->bool{ return _values[i].Type()==EValue::Bool ? _values[i].get_bool() : _values[i].get_number<uint8>(); }
		α GetBitOpt( uint i )Ι->optional<bool>{ return IsNull(i) ? optional<bool>{} : _values[i].Type()==EValue::Bool ? _values[i].get_bool() : _values[i].get_number<uint8>(); }
		α GetBytes( uint i )Ι->const vector<uint8_t>&{ return _values[i].get_bytes(); }
		α GetDouble( uint i )Ι->double{ return _values[i].get_double(); }
		α GetDoubleOpt( uint i )Ι->optional<double>{ return IsNull(i) ? optional<double>{} : _values[i].get_double(); }
		α GetInt( uint i )Ι->int64_t{ return _values[i].get_number<int64_t>(); }
		α GetInt32( uint i )Ι->int32_t{ return _values[i].get_number<int32_t>(); }
		α GetIntOpt( uint i )Ι->optional<_int>{ return IsNull(i) ? optional<_int>{} : _values[i].get_number<_int>(); }
		α GetGuid( uint i )Ε->boost::uuids::uuid{ return _values[i].get_guid(); }
		α GetString( uint i )Ι->const string&{ return IsNull(i) ? Str::Empty() : const_cast<Row*>( this )->GetString( i ); }
		α GetString( uint i )ι->string&{ if( IsNull(i) ) _values[i] = Value{string{}}; return _values[i].get_string(); }
		α GetTimePoint( uint i )Ι->DBTimePoint{ return _values[i].get_time(); }
		α GetTimePointOpt( uint i )Ι->optional<DBTimePoint>{ return IsNull(i) ? optional<DBTimePoint>{} : _values[i].get_time(); }
		α GetUInt( uint i )Ι->uint{ return _values[i].get_number<uint>(); }
		α GetUIntOpt( uint i )Ι->optional<uint>{ return IsNull(i) ? optional<uint>{} : _values[i].get_uint(); }
		α GetUInt8Opt( uint i )Ι->optional<uint8>{ return IsNull(i) ? optional<uint8>{} : _values[i].get_number<uint8>(); }
		α GetUInt16( uint i )Ε->uint16_t{ return static_cast<uint16_t>(GetUInt(i)); }
		α GetUInt32( uint i )Ε->uint32_t{ return static_cast<uint32_t>(GetUInt(i)); }
		α GetUInt32Opt( uint i )Ι->optional<uint32>{ return IsNull(i) ? optional<uint32>{} : _values[i].get_number<uint32>(); }
		α IsNull( uint i )Ι->bool{ return _values[i].Type() == EValue::Null; }
		α Size()Ι->uint{ return _values.size(); }
	private:
		vector<Value> _values;
	};
	template<> Ξ Row::Get<string>( uint i )Ι->string{ return GetString(i); }
	Ŧ Row::Get( uint i )Ι->T{ return _values[i].get_number<T>(); }
	Ŧ Row::GetOpt( uint i )Ι->optional<T>{ return IsNull(i) ? optional<T>() : Get<T>(i); }
}
#endif