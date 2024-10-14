#pragma once
#include <jde/db/usings.h>
#include <jde/framework/io/json.h>

namespace Jde::DB{
	enum class EValue:uint8{ Null, String, Bool, Int8, Int32, Int64, UInt32, UInt64, Double, Time };
	α ToType( sv typeName )ι->EType;

	struct Syntax;
	struct Value{
		using Underlying=variant<std::nullptr_t,string,bool,int8_t,int,_int,uint32_t,uint,double,DBTimePoint>;
		Value()=default;
		Value( Underlying v )ι:Variant{v}{}
		Value( EType type, const jvalue& j, SRCE )ε;

		α ToJson( jvalue& j )Ι->void;
		α ToJson()Ι->jvalue{ jvalue j; ToJson(j); return j; };

		α ToString()Ι->string;
		α TypeName()Ι->string;
		α ToUInt()Ι->uint;
		α ToInt()Ι->_int{ return (_int)ToUInt(); }
		α Type()Ι->EValue{ return (EValue)Variant.index(); }

		α get_string()Ι->const string&{ return get<string>(Variant); }
		α get_bool()Ι->bool{ return get<bool>(Variant); }
		α get_int8()Ι->int8_t{ return get<int8_t>(Variant); }
		α get_int32()Ι->int{ return get<int>(Variant); }
		α get_uint32()Ι->uint32_t{ return get<uint32_t>(Variant); }
		α get_int()Ι->_int{ return get<_int>(Variant); }
		α get_uint()Ι->uint{ return get<uint>(Variant); }
		α get_double()Ι->double{ return get<double>(Variant); }
		α get_time()Ι->DBTimePoint{ return get<DBTimePoint>(Variant); }

		α is_null()Ι->bool{ return holds_alternative<nullptr_t>(Variant); }
		α is_bool()Ι->bool{ return holds_alternative<bool>(Variant); }
		α is_double()Ι->bool{ return holds_alternative<double>(Variant); }
		α is_int32()Ι->bool{ return holds_alternative<int>(Variant); }

		α is_string()Ι->bool{ return holds_alternative<string>(Variant); }

		α set_bool( bool v )ι->void{ Variant=v; }
		Underlying Variant;
	};

	Ŧ ToValue( vec<T> x )ι->vector<Value>;

}
namespace Jde{
	Ŧ DB::ToValue( vec<T> x )ι->vector<Value>{
		vector<Value> y;
		y.reserve( x.size() );
		for( auto& i : x )
			y.push_back( Value{i} );
		return y;
	}
}