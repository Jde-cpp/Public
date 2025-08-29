#pragma once
#ifndef TO_VEC_H //gcc pragma once is not supported
#define TO_VEC_H
#include <sstream>

namespace Jde{
	template<class T>
	struct Vector : private vector<T>{
		using base=vector<T>;
		Vector()ι:base{}{}
		Vector( uint size )ι:base{ size }{}
		α at( uint index )ι->T&{ ul l{ Mutex }; return at( index, l ); }
		α at( uint index, ul& )ι->T&{ return base::at( index ); }
		α begin( sl& )Ι->typename base::const_iterator{ return base::begin(); }
		α end( sl& )Ι->typename base::const_iterator{ return base::end(); }
		α begin( ul& )ι->typename base::iterator{ return base::begin(); }
		α end( ul& )ι->typename base::iterator{ return base::end(); }

		α clear()ι{ ul _( Mutex ); base::clear(); }
		α find( const T& x )ι->optional<T>{ sl l( Mutex ); auto p = std::ranges::find(Base(), x); return p==end(l) ? nullopt : optional<T>{ *p }; }
		α erase( const T& x )ι->bool{ ul l( Mutex ); auto p = std::ranges::find(Base(), x); bool found = p!=end(l); base::erase(p); return found; }
		α	erase( function<void(const T& p)> before )ι->void;
		α	erase_if( function<bool(const T& p)> test )ι->void;
		α	erase_first( function<bool(const T& p)> test, ul& l )ι->bool;

		α push_back( const T& val )ι{ ul l( Mutex ); push_back( val, l ); }
		α push_back( const T& val, ul& _ )ι{ base::push_back( val ); }
		α push_back( T&& val )ι{ ul l( Mutex ); push_back( move(val), l ); }
		α push_back( T&& val, ul& l )ι{ base::push_back( move(val) ); }
		ψ emplace_back( Args&&... args )ι->T&{ ul l( Mutex ); return emplace_back( l, std::forward<Args>(args)... ); }
		ψ emplace_back( ul&, Args&&... args )ι->T&{ return base::emplace_back( std::forward<Args>(args)... ); }
		α reserve( uint size )ι->void{ ul l( Mutex ); reserve( size, l ); }
		α reserve( uint size, ul& l )ι->void{ base::reserve( size ); }
		α empty()Ι->uint{ sl _{Mutex}; return base::empty(); }
		α size()Ι->uint{ sl l( Mutex ); return base::size(); }
		α size( ul& )Ι->uint{ return base::size(); }
		α size( sl& )Ι->uint{ return base::size(); }
		α visit( function<void(const T& p)> f )ι->void;

		mutable std::shared_mutex Mutex;
	private:
		α Base()ι->vector<T>&{ return (vector<T>&)*this; }
	};

	Ŧ	Vector<T>::erase( function<void(const T& p)> before )ι->void{
		ul _( Mutex );
		for( auto p = base::begin(); p!=base::end(); p=base::erase(p) )
			before( *p );
	}
	Ŧ	Vector<T>::erase_if( function<bool(const T& p)> test )ι->void{
		ul _( Mutex );
		for( auto p=base::begin(); p!=base::end(); p = test( *p ) ? base::erase( p ) : std::next( p ) );
	}
	Ŧ	Vector<T>::erase_first( function<bool(const T& p)> test, ul& l )ι->bool{
		auto p = find_if( Base(), test );
		auto y = p != Base().end();
		if( y )
			Base().erase( p );
		return y;
	}
	Ŧ	Vector<T>::visit( function<void(const T& p)> f )ι->void{
		ul _( Mutex );
		for_each( Base(), f );
	}
}

//https://stackoverflow.com/questions/21806561/concatenating-strings-and-numbers-in-variadic-template-function
namespace Jde::ToVec{

	inline void Append( vector<string>& /*values*/ ){}

	template<typename Head, typename... Tail>
	void Append( vector<string>& values, Head&& h, Tail&&... t )ι;

	template<typename... Tail>
	void Append( vector<string>& values, string&& h, Tail&&... t )ι{
		values.push_back( h );
		return Apend( values, std::forward<Tail>(t)... );
	}

	template<class T> inline α ToStringT( const T& x )ι->string{
		constexpr bool StringConcept = requires(const T& t) { t.data(); t.size(); };
		if constexpr( StringConcept ){
			return string{ x.data(), x.size() };
		}
		else{
			std::ostringstream os;
			os << x;
			return os.str();
		}
	}

	Ξ FormatVectorArgs( sv fmt, const vector<string>& args )ε{
		return std::accumulate(
			std::begin( args ),
			std::end( args ),
			string{ fmt },
			[](sv toFmt, str arg){
				return fmt::vformat( toFmt, fmt::make_format_args(arg) );
			}
		);
	}

	template<typename Head, typename... Tail>
	void Append( vector<string>& values, Head&& h, Tail&&... t )ι{
		values.push_back( ToStringT(std::forward<Head>(h)) );
		return Append( values, std::forward<Tail>(t)... );
	}
}
#endif