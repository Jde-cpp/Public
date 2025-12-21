#include <jde/ql/types/Input.h>

#define let const auto

namespace Jde::QL{
	α Input::ExtrapolateVariables()Ι->jobject{
		jobject y;
		for( let& [key, value] : Args ){
			constexpr sv escape{ "\b$" };
			if( value.is_string() && value.get_string().starts_with(escape) ){
				let varName = sv{value.get_string()}.substr(2);
				let varValue = Variables->if_contains( varName );
				y.emplace( key, varValue ? *varValue : jvalue{} );
			}
			else
				y.emplace( key, value );
		}
		return y;
	}
	α Input::GetKey(SL sl)ε->DB::Key{
		let y = FindKey();
		THROW_IFSL( !y, "Could not find id or target in mutation  query: {}, variables: {}", serialize(Args), serialize(*Variables) );
		return *y;
	}
	α Input::FindKey()ι->optional<DB::Key>{
		optional<DB::Key> y;
		if( let id = FindId<uint>(); id )
			y = DB::Key{ *id };
		else if( let target = FindPtr<jstring>("target"); target )
			y = DB::Key{ string{*target} };
		return y;
	}
}