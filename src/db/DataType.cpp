#include <jde/db/DataType.h>
#include "../../../Framework/source/DateTime.h"
#include <jde/Str.h>
#include "metadata/ddl/Syntax.h"

#define let const auto

namespace Jde{
	static let& _logTag{ Logging::Tag("sql") };
	using nlohmann::json;

	α DB::ToString( const object& v )ι->string{
		string y;
		switch( (EObject)v.index() ){
			case EObject::Null: y = "null"; break;
			case EObject::String: y = get<string>( v ); break;
			case EObject::Bool: y = get<bool>( v ) ? "true" : "false"; break;
			case EObject::Int8: y = std::to_string( get<int8_t>( v ) ); break;
			case EObject::Int32: y = std::to_string( get<int>( v ) ); break;
			case EObject::Int64: y = std::to_string( get<_int>( v ) ); break;
			case EObject::UInt32: y = std::to_string( get<uint32_t>( v ) ); break;
			case EObject::UInt64: y = std::to_string( get<uint>( v ) ); break;
			case EObject::Double: y = std::to_string( get<double>( v ) ); break;
			case EObject::Time: y = ToIsoString( get<DBTimePoint>( v ) ); break;
		}
		return y;
	}

	α  DB::ToType( iv typeName )ι->DB::EType{
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
		else if( typeName=="uint32" )
			type = EType::UInt;
		else if( typeName=="uint64" )
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
		else
			TRACE( "Unknown datatype({}).", ToSV(typeName) );
		return type;
	}

	α DB::ToString( EType type, const Syntax& syntax )ι->String{
		String typeName;
		if( syntax.HasUnsigned() && type == EType::UInt ) typeName = "int unsigned";
		else if( type == EType::Int || type == EType::UInt ) typeName = "int";
		else if( syntax.HasUnsigned() && type == EType::ULong ) typeName = "bigint(20) unsigned";
		else if( type == EType::Long || type == EType::ULong ) typeName="bigint";
		else if( type == EType::DateTime ) typeName = "datetime";
		else if( type == EType::SmallDateTime )typeName = "smalldatetime";
		else if( type == EType::Float ) typeName = "float";
		else if( type == EType::SmallFloat )typeName = "real";
		else if( type == EType::VarWChar ) typeName = "nvarchar";
		else if( type == EType::WChar ) typeName = "nchar";
		else if( syntax.HasUnsigned() && type == EType::UInt16 ) typeName="smallint";
		else if( type == EType::Int16 || type == EType::UInt16 ) typeName="smallint";
		else if( syntax.HasUnsigned() && type == EType::UInt8 ) typeName =  "tinyint unsigned";
		else if( type == EType::Int8 || type == EType::UInt8 ) typeName = "tinyint";
		else if( type == EType::Guid ) typeName = "uniqueidentifier";
		else if( type == EType::VarBinary ) typeName = "varbinary";
		else if( type == EType::VarChar ) typeName = "varchar";
		else if( type == EType::NText ) typeName = "ntext";
		else if( type == EType::Text ) typeName = "text";
		else if( type == EType::Char ) typeName = "char";
		else if( type == EType::Image ) typeName = "image";
		else if( type == EType::Bit ) typeName="bit";
		else if( type == EType::Binary ) typeName = "binary";
		else if( type == EType::Decimal ) typeName = "decimal";
		else if( type == EType::Numeric ) typeName = "numeric";
		else if( type == EType::Money ) typeName = "money";
		else ERR( "Unknown datatype({})."sv, (uint)type );
		return typeName;
	}

	α DB::ToObject( EType type, const json& j, sv memberName, SL sl )ε->DB::object{
		object value{ nullptr };
		if( !j.is_null() ){
			switch( type ){
			case EType::Bit:
				THROW_IFX( !j.is_boolean(), Exception(sl, "'{}' could not convert to boolean '{}'", memberName, j.dump()) );
				value = object{ j.get<bool>() };
				break;
			case EType::Int8:
				THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to int '{}'", memberName, j.dump()) );
				value = object{ j.get<int8_t>() };
				break;
			case EType::Int16: case EType::Int: case EType::Long:
				THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to int '{}'", memberName, j.dump()) );
				value = object{ j.get<_int>() };
				break;
			case EType::UInt16: case EType::UInt: case EType::ULong:
				THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to uint '{}'", memberName, j.dump()) );
				value = object{ j.get<uint>() };
				break;
			case EType::SmallFloat: case EType::Float: case EType::Decimal: case EType::Numeric: case EType::Money:
				THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to numeric '{}'", memberName, j.dump()) );
				value = object{ j.get<double>() };
				break;
			case EType::None: case EType::Binary: case EType::VarBinary: case EType::Guid: case EType::Cursor: case EType::RefCursor: case EType::Image: case EType::Blob: case EType::TimeSpan:
				throw Exception{ sl, "EObject {} is not implemented.", (uint)type };
			case EType::VarWChar: case EType::VarChar: case EType::NText: case EType::Text: case EType::Uri:
				THROW_IFX( !j.is_string(), Exception(sl, "'{}' could not convert to string", memberName) );
				value = object{ j.get<string>() };
				break;
			case EType::WChar: case EType::UInt8: case EType::Char:
				throw Exception{ sl, "char EObject {} is not implemented.", (uint)type };
			case EType::DateTime: case EType::SmallDateTime:
				THROW_IFX( !j.is_string(), Exception(sl, "'{}' could not convert to string for datetime", memberName) );
				const string time{ j.get<string>() };
				const Jde::DateTime dateTime{ time };
				const TimePoint t = dateTime.GetTimePoint();
				value = object{ t };
				break;
			}
		}
		return value;
	}

	α DB::ToObject( EType type, const jvalue& j, sv memberName, SL sl )ε->object{
		object value{ nullptr };
		if( j.is_null() )
			return value;

		switch( type ){
			using enum EType;
		case Bit:
			THROW_IFX( !j.is_bool(), Exception(sl, "'{}' could not convert to boolean '{}'", memberName, serialize(j)) );
			value = object{ j.as_bool() };
			break;
		case Int8:
			THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to int '{}'", memberName, serialize(j)) );
			value = object{ (int8_t)j.as_int64() };
			break;
		case Int16: case Int: case Long:
			THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to int '{}'", memberName, serialize(j)) );
			value = object{ (_int)j.as_int64() };
			break;
		case UInt16: case UInt: case ULong:
			THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to uint '{}'", memberName, serialize(j)) );
			value = object{ (uint)j.as_uint64() };
			break;
		case SmallFloat: case Float: case Decimal: case Numeric: case Money:
			THROW_IFX( !j.is_number(), Exception(sl, "'{}' could not convert to numeric '{}'", memberName, serialize(j)) );
			value = object{ (double)j.as_double() };
			break;
		case None: case Binary: case VarBinary: case Guid: case Cursor: case RefCursor: case Image: case Blob: case TimeSpan:
			throw Exception{ sl, "EObject {} is not implemented.", (uint)type };
		case VarWChar: case VarChar: case NText: case Text: case Uri:
			THROW_IFX( !j.is_string(), Exception(sl, "'{}' could not convert to string", memberName) );
			value = object{ string{j.as_string()} };
			break;
		case WChar: case UInt8: case Char:
			throw Exception{ sl, "char EObject {} is not implemented.", (uint)type };
		case DateTime: case SmallDateTime:
			THROW_IFX( !j.is_string(), Exception(sl, "'{}' could not convert to string for datetime", memberName) );
			const string time{ string{j.as_string()} };
			const Jde::DateTime dateTime{ time };
			const TimePoint t = dateTime.GetTimePoint();
			value = object{ t };
			break;
		}
		return value;
	}

	α DB::ToJson( const object& v, json& j )ι->void{
		let index = (EObject)v.index();
		if( index==EObject::String )
			j = get<string>( v );
		else if( index==EObject::Null )
			j = {};
		else if( index==EObject::Bool )
			j = get<bool>( v );
		else if( index==EObject::Int64 )
			j = get<_int>( v );
		else if( index==EObject::UInt64 )
			j = get<uint>( v );
		else if( index==EObject::Int32 )
			j = get<int>( v );
		else if( index==EObject::Double )
			j = get<double>( v );
		else if( index==EObject::Time )
			j = ToIsoString( get<DB::DBTimePoint>(v) );
		else
			ERR( "{} not implemented"sv, (uint8)index );
	}
}