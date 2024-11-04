#pragma once
#ifndef ROW_H
#define ROW_H
#include "Value.h"
#include <jde/framework/str.h>

namespace Jde::DB{
	struct IRow{
		virtual ~IRow(){}
		β Move()ι->up<IRow> =0;
		β operator[]( uint value )Ε->Value=0;
		β GetBit( uint position, SRCE )Ε->bool=0;
		β GetString( uint position, SRCE )Ε->string=0;
		β GetInt( uint position, SRCE )Ε->int64_t=0;
		β GetInt32( uint position, SRCE )Ε->int32_t=0;
		β GetIntOpt( uint position, SRCE )Ε->std::optional<_int> = 0;
		β GetDouble( uint position, SRCE )Ε->double=0;
		β GetFloat( uint position, SRCE )Ε->float{ return static_cast<float>( GetDouble(position, sl) ); }
		β GetDoubleOpt( uint position, SRCE )Ε->std::optional<double> = 0;
		β GetTimePoint( uint position, SRCE )Ε->DBTimePoint=0;
		β GetTimePointOpt( uint position, SRCE )Ε->std::optional<DBTimePoint> = 0;
		β GetUInt( uint position, SRCE )Ε->uint=0;
		β GetUInt32( uint position, SRCE )Ε->uint32_t{ return static_cast<uint32_t>(GetUInt(position, sl)); }
		β GetUInt16( uint position, SRCE )Ε->uint16_t{ return static_cast<uint16_t>(GetUInt(position, sl)); }
		β GetUIntOpt( uint position, SRCE )Ε->std::optional<uint> = 0;
		Ŧ Get( uint position, SRCE )Ε->T;
		β Size()Ι->uint=0;

		friend α operator>>( const IRow& row, string& str )ε->const IRow&{ str=row.GetString(row._index++); return row; }
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
		mutable uint _index{0};
	};

	template<> Ξ IRow::Get<string>( uint position, SL sl )Ε->string{ return GetString(position, sl); }
	template<> Ξ IRow::Get<uint>( uint position, SL sl )Ε->uint{ return GetUInt(position, sl); }
	template<> Ξ IRow::Get<unsigned int>( uint position, SL sl )Ε->unsigned int{ return (unsigned int)GetUInt(position, sl); }
	template<> Ξ IRow::Get<optional<uint32>>( uint position, SL sl )Ε->optional<uint32>{ const auto p = GetUIntOpt(position, sl); return p ? optional<uint32>{static_cast<uint32>(*p)} : optional<uint32>{}; }
	template<> Ξ IRow::Get<uint8>( uint position, SL sl )Ε->uint8{ return (uint8)GetUInt16(position, sl); }
	template<> Ξ IRow::Get<long>( uint position, SL sl )Ε->long{ return (long)GetInt32(position,sl); }
	template<> Ξ IRow::Get<optional<TimePoint>>( uint position, SL sl )Ε->optional<TimePoint>{ return GetTimePointOpt(position, sl); }
}
#endif