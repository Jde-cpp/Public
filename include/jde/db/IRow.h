#pragma once
#ifndef ROW_H
#define ROW_H
#include "Value.h"
#include <jde/framework/str.h>

namespace Jde::DB{
	struct IRow{
		virtual ~IRow(){}
		β Move()ι->up<IRow> =0;
		β operator[]( uint value )Ι->const Value& = 0;
		β operator[]( uint i )ι->Value& = 0;
		β GetBit( uint i, SRCE )Ε->bool=0;
		β MoveString( uint i, SRCE )ε->string=0;
		β GetInt( uint i, SRCE )Ε->int64_t=0;
		β GetInt32( uint i, SRCE )Ε->int32_t=0;
		β GetIntOpt( uint i, SRCE )Ε->std::optional<_int> = 0;
		β GetDouble( uint i, SRCE )Ε->double=0;
		β GetFloat( uint i, SRCE )Ε->float{ return static_cast<float>( GetDouble(i, sl) ); }
		β GetDoubleOpt( uint i, SRCE )Ε->std::optional<double> = 0;
		β GetTimePoint( uint i, SRCE )Ε->DBTimePoint=0;
		β GetTimePointOpt( uint i, SRCE )Ε->std::optional<DBTimePoint> = 0;
		β GetUInt( uint i, SRCE )Ε->uint=0;
		β GetUInt32( uint i, SRCE )Ε->uint32_t{ return static_cast<uint32_t>(GetUInt(i, sl)); }
		β GetUInt16( uint i, SRCE )Ε->uint16_t{ return static_cast<uint16_t>(GetUInt(i, sl)); }
		β GetUIntOpt( uint i, SRCE )Ε->std::optional<uint> = 0;
		Ŧ Get( uint i, SRCE )Ε->T;
		β Size()Ι->uint=0;

		//friend α operator>>( const IRow& row, string& str )ε->const IRow&{ str=row.GetString(row._index++); return row; }
		friend α operator>>( const IRow& row, uint8_t& value )ε->const IRow&{ value=static_cast<uint8_t>(row.GetUInt(row._index++)); return row; }
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

	protected:
		β GetString( uint i, SRCE )Ε->string=0;
		mutable uint _index{0};
	};

	template<> Ξ IRow::Get<bool>( uint i, SL sl )Ε->bool{ return GetBit(i, sl); }
	template<> Ξ IRow::Get<string>( uint i, SL sl )Ε->string{ return GetString(i, sl); }//scaler<T> uses this.
	template<> Ξ IRow::Get<uint>( uint i, SL sl )Ε->uint{ return GetUInt(i, sl); }
	template<> Ξ IRow::Get<unsigned int>( uint i, SL sl )Ε->unsigned int{ return (unsigned int)GetUInt(i, sl); }
	template<> Ξ IRow::Get<optional<uint32>>( uint i, SL sl )Ε->optional<uint32>{ const auto p = GetUIntOpt(i, sl); return p ? optional<uint32>{static_cast<uint32>(*p)} : optional<uint32>{}; }
	template<> Ξ IRow::Get<uint8>( uint i, SL sl )Ε->uint8{ return (uint8)GetUInt16(i, sl); }
	template<> Ξ IRow::Get<long>( uint i, SL sl )Ε->long{ return (long)GetInt32(i,sl); }
	template<> Ξ IRow::Get<optional<TimePoint>>( uint i, SL sl )Ε->optional<TimePoint>{ return GetTimePointOpt(i, sl); }
}
#endif