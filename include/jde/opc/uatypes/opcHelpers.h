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
	struct NodeId;
	α FindDataType( const NodeId& nodeId )ι->UA_DataType*;
	Ŧ Zero( T& x )ι->void{ ::memset( &x, 0, sizeof(T) ); }
	constexpr α operator ""_uv( const char* x, uint len )ι->UA_String{ return UA_String{ len, static_cast<UA_Byte*>((void*)x) }; } //(UA_Byte*) gcc error
	Ξ ToJson( UA_UInt64 v )ι->jobject{ return jobject{ {"high", v>>32}, {"low", v&0xFFFFFFFF}, {"unsigned",true} }; };
	Ξ ToJson( UA_Int64 v )ι->jobject{ return jobject{ {"high", v>>32}, {"low", v&0xFFFFFFFF}, {"unsigned",false} }; };
	Ξ ToGuid( const UA_Guid& ua )ι->uuid{ //data1/2/3 are native-endian in UA_Guid, big-endian (RFC 4122) in uuid.
		uuid y;
		y.data[0]=(uint8_t)(ua.data1>>24); y.data[1]=(uint8_t)(ua.data1>>16); y.data[2]=(uint8_t)(ua.data1>>8); y.data[3]=(uint8_t)ua.data1;
		y.data[4]=(uint8_t)(ua.data2>>8); y.data[5]=(uint8_t)ua.data2;
		y.data[6]=(uint8_t)(ua.data3>>8); y.data[7]=(uint8_t)ua.data3;
		::memcpy( y.data+8, ua.data4, sizeof(ua.data4) );
		return y;
	}
	Ξ ToUAGuid( const uuid& id )ι->UA_Guid{
		UA_Guid ua;
		ua.data1 = (UA_UInt32)id.data[0]<<24 | (UA_UInt32)id.data[1]<<16 | (UA_UInt32)id.data[2]<<8 | id.data[3];
		ua.data2 = (UA_UInt16)( id.data[4]<<8 | id.data[5] );
		ua.data3 = (UA_UInt16)( id.data[6]<<8 | id.data[7] );
		::memcpy( ua.data4, id.data+8, sizeof(ua.data4) );
		return ua;
	}
	Ξ ToJson( UA_Guid v )ι->jstring{ return jstring{ Jde::ToString(ToGuid(v)) }; }
	Ξ ByteStringToJson( const UA_ByteString& v )ι->jstring{ string hex; hex.reserve( v.length*2 ); boost::algorithm::hex_lower( ToSV(v), std::back_inserter(hex) ); return jstring{hex}; }//TODO combine with Str::
	Ξ ToGuid( string x, UA_Guid& ua )ε->void{
		std::erase( x, '-' );
		try{
			ua = ToUAGuid( boost::lexical_cast<boost::uuids::uuid>(x) );
		}
		catch( const boost::bad_lexical_cast& ){
			THROW( "Could not parse guid: '{}'.", x );
		}
	}
	Ξ ToBinaryString( const UA_Guid& ua )ι->string{ return {(const char*)&ua, sizeof(UA_Guid)}; }
	using ByteStringPtr = up<UA_ByteString,decltype(&UA_ByteString_delete)>;
	Ŧ ToUAByteString( const T& bytes )ι->ByteStringPtr{
		ByteStringPtr y{ UA_ByteString_new(), UA_ByteString_delete };
		if( bytes.size() ){
			UA_ByteString_allocBuffer( y.get(), bytes.size() );
			memcpy( y->data, bytes.data(), bytes.size() );
		}
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

	Τ struct Iterable final : noncopyable{
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