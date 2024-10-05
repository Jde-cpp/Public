#pragma once
#include <variant>
DISABLE_WARNINGS
#include <nlohmann/json.hpp>
ENABLE_WARNINGS
#include <jde/Str.h>
#include <jde/log/Log.h>
#include <jde/Assert.h>

namespace Jde::DB{

	struct Syntax;
	using nlohmann::json;
	using DBClock=std::chrono::system_clock;
	using DBTimePoint=DBClock::time_point;
	enum class EObject:uint8{ Null, String, Bool, Int8, Int32, Int64, UInt32, UInt64, Double, Time };
	using object=std::variant<std::nullptr_t,string,bool,int8_t,int,_int,uint32_t,uint,double,DBTimePoint>;
	α ToString( const object& parameter )ι->string;

	enum class EType:uint8{None,Int16,Int,UInt,SmallFloat,Float,Bit,Decimal,Int8,Long,ULong,Guid,Binary,VarBinary,VarWChar,Numeric,DateTime,Cursor,VarChar,RefCursor,SmallDateTime,WChar,NText,Text,Image,Blob,Money,Char,TimeSpan,Uri,UInt8,UInt16 };

	α ToType( iv typeName )ι->EType;
	α ToString( EType type, const Syntax& syntax )ι->String;

	α ToObject( EType type, const nlohmann::json& j, sv memberName, SRCE )ε->object;
	α ToObject( EType type, const jvalue& j, sv memberName, SRCE )ε->object;
	α ToJson( const object& obj, json& j )ι->void;
	Ξ ToJson( const object& obj )ι->json{ json j; ToJson(obj,j); return j; };
#define var const auto
	Ξ ToUInt( const object& x )ι->uint{
		uint y; var i = (EObject)x.index();
		if( i==EObject::Int32 )
			y = get<int>( x );
		else if( i==EObject::Int64 )
			y = get<_int>( x );
		else if( i==EObject::UInt64 )
			y = get<uint>( x );
		else
			ASSERT( y=0 );
		return y;
	}
	Ξ ToInt( const object& x )ι->_int{ return (_int)ToUInt(x); }
}
#undef var