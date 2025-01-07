#include <jde/db/Value.h>
#include <jde/db/usings.h>
#include <jde/db/generators/Syntax.h>
#include "../../../../Framework/source/DateTime.h"

#define let const auto

namespace Jde::DB{
	using namespace Json;
	constexpr ELogTags _tags{ ELogTags::Sql };
	constexpr array<sv,10> EValueStrings = { "null", "string", "bool", "int8", "int32", "int64", "uint32", "uint64", "double", "time" };

	α FromJson( EType type, const jvalue& j, SL sl )->Value::Underlying{
		Value::Underlying value{ nullptr };
		if( j.is_null() )
			return value;

		switch( type ){
			using enum EType;
		case VarWChar: case VarChar: case NText: case Text: case Uri: value = AsString( j ); break;
		case Bit: value = AsBool(j, sl); break;
		case Int8: value = AsNumber<int8>(j, sl); break;
		case Int16: case Int: value = AsNumber<int>( j ); break;
		case Long: value = AsNumber<_int>( j ); break;
		case UInt8: value = AsNumber<uint8>( j ); break;
		case UInt16: value = AsNumber<uint16>( j ); break;
		case UInt: value = AsNumber<uint32>( j ); break;
		case ULong: value = AsNumber<uint>( j ); break;
		case SmallFloat: case Float: case Decimal: case Numeric: case Money: value = AsNumber<double>( j ); break;
		case DateTime: case SmallDateTime: value = AsString( j ); break; //should be $now
		case None: case Binary: case VarBinary: case Guid: case Cursor: case RefCursor: case Image: case Blob: case TimeSpan:
			throw Exception{ sl, "EValue {} is not implemented.", (uint)type };
		case WChar: case Char: default:
			throw Exception{ sl, "char EValue {} is not implemented.", (uint)type };
		}
		return value;
	}

	Value::Value( EType type, const jvalue& j, SL sl )ε:
		Variant{ FromJson(type, j, sl) }
	{}

	α Value::ToString()Ι->string{
		string y;
		using std::to_string;
		switch( Type() ){
			using enum EValue;
			case Null: y = "null"; break;
			case String: y = get_string(); break;
			case Bool: y = get_bool() ? "true" : "false"; break;
			case Int8: y = to_string( get_int8() ); break;
			case Int32: y = to_string( get_int32() ); break;
			case Int64: y = to_string( get_int() ); break;
			case UInt32: y = to_string( get_uint32() ); break;
			case UInt64: y = to_string( get_uint() ); break;
			case Double: y = to_string( get_double() ); break;
			case Time: y = ToIsoString( get_time() ); break;
		}
		return y;
	}
	α Value::TypeName()Ι->string{ return FromEnum( EValueStrings, Type() ); }

	α Value::ToUInt()Ι->uint{
		uint y{};
		using enum EValue;
		if( Type()==Int32 )
			y = get_int32();
		else if( Type()==Int64 )
			y = get_int();
		else if( Type()==UInt64 )
			y = get_uint();
		return y;
	}

	α Value::Move()ι->jvalue{
		jvalue y;
		if( Type()==EValue::String )
			y = std::get<string>( move(Variant) );
		else
			ToJson( y );
		return y;
	}

	α Value::ToJson( jvalue& j )Ι->void{
		switch( Type() ){
			using enum EValue;
		case String: j = get_string(); break;
		case Null: j = nullptr; break;
		case Bool: j = get_bool(); break;
		case Int64: j = get_int(); break;
		case UInt64: j = get_uint(); break;
		case Int32: j = get_int32(); break;
		case Double: j = get_double(); break;
		case Time: j = ToIsoString( get_time() ); break;
		default: Error{ _tags, "Unknown type({}).", TypeName() };
		}
	}
}
namespace Jde{
	α DB::ToType( sv csTypeName )ι->DB::EType{
		iv typeName{ ToIV(csTypeName) };
		//String typeName{ t };
		EType type{ EType::None };
		if( typeName=="dateTime" )
			type=EType::DateTime;
		else if( typeName=="smallDateTime" )
			type=EType::SmallDateTime;
		else if( typeName=="float" )
			type=EType::Float;
		else if( typeName=="real" )
			type=EType::SmallFloat;
		else if( typeName=="bool" )
			type=EType::Bit;
		else if( typeName=="int" )
			type = EType::Int;
		else if( typeName=="uint" )
			type = EType::UInt;
		else if( typeName=="uint64" || typeName=="ULong" )
			type = EType::ULong;
		else if( typeName=="Long" )
			type = EType::Long;
		else if( typeName=="nvarchar" )
			type = EType::VarWChar;
		else if(typeName=="nchar")
			type = EType::WChar;
		else if( typeName=="smallint" )
			type = EType::Int16;
		else if( typeName=="uint16" )
			type = EType::UInt16;
		else if( typeName=="int8" )
			type = EType::Int8;
		else if( typeName=="uint8" )
			type = EType::UInt8;
		else if( typeName=="guid" )
			type = EType::Guid;
		else if(typeName=="varbinary")
			type = EType::VarBinary;
		else if( typeName=="varchar" )
			type = EType::VarChar;
		else if( typeName=="ntext" )
			type = EType::NText;
		else if( typeName=="text" )
			type = EType::Text;
		else if( typeName=="char" )
			type = EType::Char;
		else if( typeName=="image" )
			type = EType::Image;
		else if( typeName=="bit" )
			type = EType::Bit;
		else if( typeName=="binary" )
			type = EType::Binary;
		else if( typeName=="decimal" )
			type = EType::Decimal;
		else if( typeName=="numeric" )
			type = EType::Numeric;
		else if( typeName=="money" )
			type = EType::Money;
		else if( typeName=="Uri" )
			type = EType::Uri;
		else
			Trace{ _tags, "Unknown datatype({}).", typeName };
		return type;
	}
}