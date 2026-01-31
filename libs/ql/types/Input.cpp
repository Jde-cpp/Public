#include <jde/ql/types/Input.h>

#define let const auto

namespace Jde::QL{
	α Input::ExtrapolateVariables()Ι->jobject{
		auto extrapolateString = [this]( const jstring& s )ι->jvalue{
			let varName = sv{ s }.substr( 2 );
			let varValue = Variables->if_contains( varName );
			return varValue ? *varValue : jvalue{};
		};
		function<jobject( const jobject& )> extrapolate = [&]( const jobject& o )ι->jobject {
			jobject y;
			for( let& [key, value] : o ){
				constexpr sv escape{ "\b$" };
				if( value.is_string() && value.get_string().starts_with(escape) )
					y.emplace( key, extrapolateString(value.get_string()) );
				else if( value.is_object() )
					y.emplace( key, extrapolate(Json::AsObject(value)) );
				else if( value.is_array() ){
					jarray newArray;
					for( auto&& item : value.get_array() ){
						if( item.is_string() && item.get_string().starts_with(escape) )
							newArray.emplace_back( extrapolateString(item.get_string()) );
						else if( item.is_object() )
							newArray.emplace_back( extrapolate(item.get_object()) );
						else
							newArray.emplace_back( item );
					}
					y.emplace( key, newArray );
				}
				else
					y.emplace( key, value );
			}
			return y;
		};
		return extrapolate( Args );
	}
	α Input::GetKey( SL sl )Ε->DB::Key{
		let y = FindKey();
		THROW_IFSL( !y, "Could not find id or target in mutation  query: {}, variables: {}", serialize(Args), serialize(*Variables) );
		return *y;
	}
	α Input::FindKey()Ι->optional<DB::Key>{
		optional<DB::Key> y;
		if( let id = FindId<uint>(); id )
			y = DB::Key{ *id };
		else if( let target = FindPtr<jstring>("target"); target )
			y = DB::Key{ string{*target} };
		return y;
	}
}