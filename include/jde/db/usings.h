#pragma once

namespace Jde::DB{
	using DBClock=std::chrono::system_clock;
	using DBTimePoint=DBClock::time_point;

	enum class EType : uint8{None,Int16,Int,UInt,SmallFloat,Float,Bit,Decimal,Int8,Long,ULong,Guid,Binary,VarBinary,VarWChar,Numeric,DateTime,Cursor,VarChar,RefCursor,SmallDateTime,WChar,NText,Text,Image,Blob,Money,Char,TimeSpan,Uri,UInt8,UInt16 };
}