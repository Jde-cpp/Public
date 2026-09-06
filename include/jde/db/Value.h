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

		//The heap alternatives - string and bytes - by reference.  A mismatch ASSERTs (log-only, like TryGetValue) and
		//answers a reference to an empty T: thread-local and cleared on every hand-out, so a caller that writes through it
		//neither races another thread nor poisons the next mismatch (B8 - it used to be one process-wide static per T).
		Ŧ TryGetHeap()Ι->const T&{
			auto p = std::get_if<T>( &Variant );
			ASSERT_DESC( p, Ƒ("Value is a '{}'", TypeName()) );
			if( p )
				return *p;
			thread_local T fallback;
			fallback = T{};
			return fallback;
		}

		α get_string()Ι->const string&{ return TryGetHeap<string>(); }
		α get_string()ι->string&;
		α get_bytes()Ι->const vector<uint8_t>&{ return TryGetHeap<vector<uint8_t>>(); }
		α get_bytes()ι->vector<uint8_t>&;

		//The exact alternative or a default, by value - the counterpart to TryGetHeap for the types cheap enough to copy.
		//The typed get_* below are one-liners over this and must not route through Get<T>/get_number: that is a cycle
		//with no bottom, and it presents as a stack overflow rather than a bad value.  (get_number itself visits the
		//variant - db-refactor A2 - so it no longer depends on them.)
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
		Ŧ get_number()Ι->T requires std::is_arithmetic_v<T>; //any arithmetic alternative, converted; anything else ASSERTs and answers T{}.
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
		α is_null()Ι->bool{ return holds_alternative<nullptr_t>(Variant); }
		α is_string()Ι->bool{ return holds_alternative<string>(Variant); }

		α set_bool( bool v )ι->void{ Variant=v; }
		α operator=( uint v )ι->Value&{ Variant=v; return *this; }
		α operator==( const Value& r )Ι->bool{ return Variant==r.Variant; }
		Underlying Variant;
	};

	Ŧ ToValue( vec<T> x )ι->vector<Value>;
	Ŧ ToValue( const flat_set<T>& x )ι->vector<Value>;
	//A visit, not a switch over Type(): the arithmetic alternatives (bool, the integers, double) convert with one
	//static_cast, and adding an alternative to Underlying is a compile error here rather than a silently-missed case.
	Ŧ Value::get_number()Ι->T requires std::is_arithmetic_v<T>{
		return std::visit( [&]<class U>( const U& v )->T {
			if constexpr( std::is_arithmetic_v<U> )
				return static_cast<T>( v );
			else{
				ASSERT_DESC( false, Ƒ("Value is a '{}', not a number", TypeName()) );
				return T{};
			}
		}, Variant );
	}

	//the mutable pair adds back what the const one took off - the one direction a const_cast is for (C).
	Ξ Value::get_string()ι->string&{ return const_cast<string&>( std::as_const(*this).get_string() ); }
	Ξ Value::get_bytes()ι->vector<uint8_t>&{ return const_cast<vector<uint8_t>&>( std::as_const(*this).get_bytes() ); }
	Ξ Value::get_guid()Ι->boost::uuids::uuid{
		let& bytes = get_bytes();
		constexpr uint size{ boost::uuids::uuid::static_size() };
		ASSERT_DESC( bytes.size()==size, Ƒ("Guid blob is {} bytes, expected {}", bytes.size(), size) );
		boost::uuids::uuid u{};
		std::copy_n( bytes.begin(), std::min(bytes.size(), size), u.begin() );
		return u;
	}

	Ŧ Value::Get()Ι->T{
		if constexpr( std::same_as<T,string> )
			return get_string();
		else if constexpr( std::same_as<T,DBTimePoint> )
			return get_time();
		else
			return get_number<T>();
	}
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