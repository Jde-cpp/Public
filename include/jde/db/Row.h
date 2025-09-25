#pragma once
#ifndef ROW_H
#define ROW_H
#include "Value.h"
#include <jde/framework/str.h>
#include "exports.h"

#define Φ ΓDB auto
namespace Jde::DB{
	struct Row final/*: IRow*/ {
		Row( vector<Value>&& values )ι:_values{move(values)}{}
		α operator[]( uint i )Ι->const Value&{ return _values[i]; }
		α operator[]( uint i )ι->Value&{ return _values[i]; }
		Ŧ Get( uint i )Ι->T;
		Ŧ GetOpt( uint i )Ι->optional<T>;
		Φ GetBit( uint i )Ι->bool;
		Φ GetBitOpt( uint i )Ι->optional<bool>;
		α GetBytes( uint i )Ι->const vector<uint8_t>&{ return _values[i].get_bytes(); }
		α GetDouble( uint i )Ι->double{ return _values[i].get_double(); }
		α GetDoubleOpt( uint i )Ι->optional<double>{ return IsNull(i) ? optional<double>{} : _values[i].get_double(); }
		α GetInt32( uint i )Ι->int32_t{ return _values[i].get_number<int32_t>(); }
		α GetInt32Opt( uint i )Ι->optional<int32_t>{ return IsNull(i) ? optional<int32_t>{} : _values[i].get_number<int32_t>(); }
		α GetInt( uint i )Ι->int64_t{ return _values[i].get_number<int64_t>(); }
		α GetIntOpt( uint i )Ι->optional<int64_t>{ return IsNull(i) ? optional<int64_t>{} : _values[i].get_number<int64_t>(); }
		α GetGuid( uint i )Ε->boost::uuids::uuid{ return _values[i].get_guid(); }
		α GetString( uint i )Ι->const string&{ return IsNull(i) ? Str::Empty() : const_cast<Row*>( this )->GetString( i ); }
		α GetString( uint i )ι->string&{ if( IsNull(i) ) _values[i] = Value{string{}}; return _values[i].get_string(); }
		α GetTimePoint( uint i )Ι->DBTimePoint{ return _values[i].get_time(); }
		α GetTimePointOpt( uint i )Ι->optional<DBTimePoint>{ return IsNull(i) ? optional<DBTimePoint>{} : _values[i].get_time(); }
		α GetUInt( uint i )Ε->uint{ return _values[i].get_number<uint>(); }
		α GetUIntOpt( uint i )Ι->optional<uint>{ return IsNull(i) ? optional<uint>{} : _values[i].get_uint(); }
		α GetUInt8Opt( uint i )Ι->optional<uint8>{ return IsNull(i) ? optional<uint8>{} : _values[i].get_number<uint8>(); }
		α GetUInt16( uint i )Ε->uint16_t{ return static_cast<uint16_t>(GetUInt(i)); }
		α GetUInt16Opt( uint i )Ι->optional<uint16_t>{ return IsNull(i) ? optional<uint32_t>{} : _values[i].get_number<uint16_t>(); }
		α GetUInt32( uint i )Ε->uint32_t{ return static_cast<uint32_t>(GetUInt(i)); }
		α GetUInt32Opt( uint i )Ι->optional<uint32_t>{ return IsNull(i) ? optional<uint32_t>{} : _values[i].get_number<uint32_t>(); }
		Φ IsNull( uint i )Ι->bool;
		α Size()Ι->uint{ return _values.size(); }
	private:
		vector<Value> _values;
	};
	template<> Ξ Row::Get<string>( uint i )Ι->string{ return GetString(i); }
	Ŧ Row::Get( uint i )Ι->T{ return _values[i].get_number<T>(); }
	Ŧ Row::GetOpt( uint i )Ι->optional<T>{ return IsNull(i) ? optional<T>() : Get<T>(i); }
}
#undef Φ
#endif