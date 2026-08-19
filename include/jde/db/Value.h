#pragma once
#include "jde/fwk/str.h"
#include <boost/uuid/uuid.hpp>
#include <jde/fwk/io/json.h>
#include <jde/db/exports.h>
#include <jde/db/usings.h>
#include <jde/db/Key.h>

#define let const auto
namespace Jde::DB{
	using uuid=boost::uuids::uuid;
	struct Key;
	enum class EValue:uint8{ Null, String=1, Bool=2, Int8=3, Int32=4, Int64=5, UInt32=6, UInt64=7, Double=8, Time=9, Bytes=10 };
	α ΓDB ToType( sv typeName )ι->EType;

	struct Syntax;
	struct ΓDB Value{
		using Underlying=variant<std::nullptr_t,string,bool,int8_t,int,_int,uint32_t,uint,double,DBTimePoint,vector<uint8_t>>;
		Value()=default;
		Value( Underlying v )ι:Variant{move(v)}{}
		Value( Underlying v, Underlying nullValue )ι:Variant{ v==nullValue ? Underlying{nullptr} : move(v) }{}
		Value( uuid guid )ι:Variant{vector<uint8_t>( (uint8_t*)guid.data(), (uint8_t*)guid.data()+16 )}{}
		Value( EType type, const jvalue& j, SRCE )ε;
		Ω FromKey( Key key )ι->Value{ return key.IsPK() ? Value{key.PK()} : Value{move(key.NK())}; }

		α ToJson( jvalue& j )Ι->void;
		α ToJson()Ι->jvalue{ jvalue v; ToJson(v); return v; };
		α Move()ι->jvalue;

		α ToString()Ι->string;
		α TypeName()Ι->string;
		α ToUInt()Ι->uint;
		α ToInt()Ι->_int{ return (_int)ToUInt(); }
		α Type()Ι->EValue{ return (EValue)Variant.index(); }

		Ŧ TryGetHeap()ι->T&;

		α get_string()Ι->const string&{ return const_cast<Value*>(this)->get_string(); }
		α get_string()ι->string&;
		α get_bytes()Ι->const vector<uint8_t>&{ return const_cast<Value*>(this)->get_bytes(); }
		α get_bytes()ι->vector<uint8_t>&;

		//The exact alternative or a default, by value - the counterpart to TryGetHeap for the types cheap enough to copy.
		//These are get_number's base case, which it reaches through GET(x), so they must not route back through
		//Get<T>/get_number:  that is a cycle with no bottom, and it presents as a stack overflow rather than a bad value.
		Ŧ TryGetValue()Ι->T{
			auto p = std::get_if<T>( &Variant );
			ASSERT_DESC( p, Ƒ("Value is a '{}'", TypeName()) );
			return p ? *p : T{};
		}
		α get_bool()Ι->bool{ return TryGetValue<bool>(); }
		α get_double()Ι->double{ return TryGetValue<double>(); }
		α get_guid()Ι->boost::uuids::uuid;
 		α get_int8()Ι->int8_t{ return TryGetValue<int8_t>(); }
		α get_int32()Ι->int{ return TryGetValue<int>(); }
		α get_int()Ι->_int{ return TryGetValue<_int>(); }
		Ŧ get_number()Ι->T requires std::is_arithmetic_v<T>;
		Ŧ Get()Ι->T;
		α get_uint32()Ι->uint32_t{ return TryGetValue<uint32_t>(); }
		α get_uint()Ι->uint{ return TryGetValue<uint>(); }
		α get_time()Ι->DBTimePoint{ return TryGetValue<DBTimePoint>(); }
		α is_bool()Ι->bool{ return holds_alternative<bool>(Variant); }
		α is_number()Ι->bool{
			switch( Type() ){
				using enum EValue;
				case Int8: case Int32: case Int64: case UInt32: case UInt64: case Double: return true;
				default: return false;
			}
		}
		α is_double()Ι->bool{ return holds_alternative<double>(Variant); }
		α is_int32()Ι->bool{ return holds_alternative<int>(Variant); }
		α is_null()Ι->bool{ return holds_alternative<nullptr_t>(Variant); }
		α is_string()Ι->bool{ return holds_alternative<string>(Variant); }

		α set_bool( bool v )ι->void{ Variant=v; }
		α operator=( uint v )ι->Value&{ Variant=v; return *this; }
		α operator==( const Value& r )Ι->bool{ return Variant==r.Variant; }
		Underlying Variant;
		static string _errorString;
		static vector<uint8_t> _errorBytes;
	};

	Ŧ ToValue( vec<T> x )ι->vector<Value>;
	Ŧ ToValue( const flat_set<T>& x )ι->vector<Value>;
#define GET(x) static_cast<T>( get_##x() )
	Ŧ Value::get_number()Ι->T requires std::is_arithmetic_v<T>{
		switch( Type() ){
			using enum EValue;
		case Bool: return GET( bool ) ? 1 : 0;
		case Int8: return GET(int8);
		case Int32: return GET(int32);
		case Int64: return GET(int);
		case UInt32: return GET(uint32);
		case UInt64: return GET(uint);
		case Double: return GET(double);
		default:
			ASSERT_DESC( false, Ƒ("Value is a '{}', not a number", TypeName()) );
			return T{};
		}
	}

	template<> Ξ Value::TryGetHeap<string>()ι->string&{
		auto p = std::get_if<string>( &Variant );
		ASSERT_DESC( p, Ƒ("Value is a '{}'", TypeName()) );
		return p ? *p : _errorString;
	}
	template<> Ξ Value::TryGetHeap<vector<uint8_t>>()ι->vector<uint8_t>&{
		auto p = std::get_if<vector<uint8_t>>( &Variant );
		ASSERT_DESC( p, Ƒ("Value is a '{}'", TypeName()) );
		return p ? *p : _errorBytes;
	}
	Ξ Value::get_string()ι->string&{ return TryGetHeap<string>(); }
	Ξ Value::get_bytes()ι->vector<uint8_t>& { return TryGetHeap<vector<uint8_t>>(); }
	Ξ Value::get_guid()Ι->boost::uuids::uuid{
		let& bytes = get_bytes();
		constexpr uint size{ boost::uuids::uuid::static_size() };
		ASSERT_DESC( bytes.size()==size, Ƒ("Guid blob is {} bytes, expected {}", bytes.size(), size) );
		boost::uuids::uuid u{};
		std::copy_n( bytes.begin(), std::min(bytes.size(), size), u.begin() );
		return u;
	}

	Ŧ Value::Get()Ι->T{
		if constexpr( std::same_as<T,string> ){
			return get_string();
		}
		else
			return get_number<T>();
	}
#undef GET
}
namespace Jde{
	Ŧ DB::ToValue( vec<T> x )ι->vector<Value>{
		vector<Value> y;
		y.reserve( x.size() );
		for( auto& i : x )
			y.push_back( Value{i} );
		return y;
	}
	Ŧ DB::ToValue( const flat_set<T>& x )ι->vector<Value>{
		vector<Value> y;
		y.reserve( x.size() );
		for( auto& i : x )
			y.push_back( Value{i} );
		return y;
	}
}
#undef let