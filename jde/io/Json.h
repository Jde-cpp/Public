#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
namespace Jde::Json{
	Ŧ Routine( std::function<T()>&& f, SRCE )ε->T{
		try{		
			return f();
		}
		catch( IException& e ){
			e.Throw();
		}
		catch( const json::basic_json::type_error& e ){
			throw Exception( sl, "json type_error: {}", e.what() );
		}
		catch( const nlohmann::detail::parse_error& e ){
			throw Exception{ sl, "detail::parse_error: {}", e.what() };
		}
		catch( const json::exception& e ){
			throw Exception{ sl, "json::exception: {}", e.what() };
		}
		catch( const nlohmann::detail::exception& e ){
			throw Exception{ sl, "detail::exception exception: {}", e.what() };
		}
		catch( const std::exception& e ){
			throw Exception{ sl, "json std::exception: {}", e.what() };
		}
	}

	template<class T=string> α Get( const json& j, str key, SRCE )ε->T{
		auto p = j.find( key );
		return p!=j.end() ? Routine<T>( [&](){return p->get<T>();}, sl ) : T{};
	}

	template<class T=string> α Getε( const json& j, str key, SRCE )ε->T{
		auto p = j.find( key );
		if( p==j.end() )
			throw Exception{ sl, "Could not find '{}' in '{}'", key, j.dump() };
		return Routine<T>( [&](){return p->get<T>();}, sl );
	}

	template<class T=string> α Getε( const json& j, const vector<sv>& keys, SRCE )ε->T{
		THROW_IFSL( j.is_null(), "j.is_null()" );
		THROW_IFSL( keys.size()==0, "keys.size()==0" );
		return Routine<T>( [&](){
			json::const_iterator p{ j.end() };
			const json* parent = &j;
			for( auto& key : keys ){
				p = parent->find( key );
				THROW_IFSL( p==parent->end(), "Could not find '{}' in '{}'", Str::AddSeparators(keys,"\\"), j.dump() );
				parent = &*p;
			}
			return p->get<T>();
		}, sl );
	}

	Ξ Parse( str j, SRCE )ε{
		try{
			return Routine<json>( [&](){return json::parse(j);}, sl );
		}
		catch( IException& e ){
			e.PrependWhat( Jde::format("{} - ", j) );
			e.Throw();
		}
	}
#pragma GCC diagnostic pop
}