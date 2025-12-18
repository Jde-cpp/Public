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
}