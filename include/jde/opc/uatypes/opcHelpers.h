#pragma once
#ifndef IOT_UA_HELPERS_H
#define IOT_UA_HELPERS_H
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <jde/fwk/chrono.h>
#include "../usings.h"
#include "UAString.h"

#define let const auto
namespace Jde::Opc{
	Ŧ Zero( T& x )ι->void{ ::memset( &x, 0, sizeof(T) ); }
	constexpr α operator "" _uv( const char* x, uint len )ι->UA_String{ return UA_String{ len, static_cast<UA_Byte*>((void*)x) }; } //(UA_Byte*) gcc error
	Ξ ToJson( UA_UInt64 v )ι->jobject{ return jobject{ {"high", v>>32}, {"low", v&0xFFFFFFFF}, {"unsigned",true} }; };
	Ξ ToJson( UA_Int64 v )ι->jobject{ return jobject{ {"high", v>>32}, {"low", v&0xFFFFFFFF}, {"unsigned",false} }; };
#pragma GCC diagnostic ignored "-Wclass-memaccess"
	Ξ ToJson( UA_Guid v )ι->jstring{ uuid id; memcpy(&id.data, &v, id.size() ); return jstring{ boost::uuids::to_string(id) }; }
	Ξ ByteStringToJson( const UA_ByteString& v )ι->jstring{ string hex; hex.reserve( v.length*2 ); boost::algorithm::hex_lower( ToSV(v), std::back_inserter(hex) ); return jstring{hex}; }//TODO combine with Str::
	Ξ ToGuid( string x, UA_Guid& ua )ι->void{ std::erase( x, '-' ); let uuid{boost::lexical_cast<boost::uuids::uuid>(x)}; ::memcpy( &ua, &uuid, sizeof(UA_Guid) ); }
	Ξ ToGuid( UA_Guid ua )ι->uuid{ uuid uuid; ::memcpy( &uuid, &ua, sizeof(UA_Guid) ); return uuid; }
	Ξ ToBinaryString( const UA_Guid& ua )ι->string{ return {(const char*)&ua, sizeof(UA_Guid)}; }
	using ByteStringPtr = up<UA_ByteString,decltype(&UA_ByteString_delete)>;
	Ŧ ToUAByteString( const T&& bytes )->ByteStringPtr{
		ByteStringPtr y = up<UA_ByteString,decltype(&UA_ByteString_delete)>{ UA_ByteString_new(), UA_ByteString_delete };
		UA_ByteString_allocBuffer( y.get(), bytes.size() );
		memcpy( y->data, bytes.data(), bytes.size() );
		return y;
	};
	Ξ FromByteString( const UA_ByteString& bytes )ι->vector<uint8_t>{
		vector<uint8_t> y;
		if( bytes.length ){
			y.resize( bytes.length );
			memcpy( y.data(), bytes.data, bytes.length );
		}
		return y;
	};

	Τ struct Iterable final : boost::noncopyable{
		Iterable( T* begin, uint size )ι:_begin{begin}, _size{size}{}
	  α begin()Ι->T*{ return _size ? _begin : end(); }
		α cbegin()Ι->const T*{ return begin(); }
	  α end()Ι->T*{ return _begin+_size; }
		α cend()Ι->const T*{ return end(); }
	private:
		T* _begin;
		const uint _size;
	};
}
#undef let
#endif