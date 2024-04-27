#pragma once
#ifndef IOT_UA_HELPERS_H
#define IOT_UA_HELPERS_H
#include <boost/algorithm/hex.hpp>
#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>

#define var const auto
namespace Jde::Iot{

	Ξ ToSV( const UA_String& s )ι->sv{ return sv{ (const char*)s.data, s.length }; }
	Ξ ToUV( sv s )ι->UA_String{ return { s.size(), (UA_Byte*)s.data() }; }
	//Ξ mum( sv s )ι->UA_String{ return { s.size(), (UA_Byte*)s.data() }; }
	Ŧ Zero( T& x )ι->void{ ::memset( &x, 0, sizeof(T) ); }
	constexpr α operator "" _uv( const char* x, uint len )ι->UA_String{ return UA_String{ len, static_cast<UA_Byte*>((void*)x) }; } //(UA_Byte*) gcc error
	Ξ ToJson( UA_UInt64 v )ι->json{ return json{ {"high", v>>32}, {"low", v&0xFFFFFFFF}, {"unsigned",true} }; };
	Ξ ToJson( UA_Int64 v )ι->json{ return json{ {"high", v>>32}, {"low", v&0xFFFFFFFF}, {"unsigned",false} }; };
	Ξ ToJson( UA_Guid v )ι->json{ boost::uuids::uuid id; memcpy(&id.data, &v, id.size() ); return json{ boost::uuids::to_string(id) }; }
	Ξ ByteStringToJson( const UA_ByteString& v )ι->json{ string hex; hex.reserve( v.length*2 ); boost::algorithm::hex_lower( ToSV(v), std::back_inserter(hex) ); return json{hex}; }
	Ξ ToGuid( string x, UA_Guid& ua )ι->void{ std::erase( x, '-' ); var uuid{boost::lexical_cast<boost::uuids::uuid>(x)}; ::memcpy( &ua, &uuid, sizeof(UA_Guid) ); }
	Ξ ToBinaryString( const UA_Guid& ua )ι->string{ return {(const char*)&ua, sizeof(UA_Guid)}; }
	using ByteStringPtr = up<UA_ByteString,decltype(&UA_ByteString_delete)>;
	Ξ ToUAByteString( const vector<byte>&& bytes )->ByteStringPtr{
		ByteStringPtr y = up<UA_ByteString,decltype(&UA_ByteString_delete)>{ UA_ByteString_new(), UA_ByteString_delete };
		UA_ByteString_allocBuffer( y.get(), bytes.size() );
		memcpy( y->data, bytes.data(), bytes.size() );
		return y;
	};

	Τ struct Iterable{
		Iterable( T* begin, uint size )ι:_begin{begin}, _size{size}{}
	  α begin()Ι->T*{ return _size ? _begin : end(); }
		α cbegin()Ι->const T*{ return begin(); }
	  α end()Ι->T*{ return _begin+_size; }
		α cend()Ι->const T*{ return end(); }
	private:
		T* _begin;
		const uint _size;
	};

	struct UADateTime
	{
		UADateTime( const UA_DateTime& dt )ι:_dateTime{dt}{}
		α ToJson()Ι->json{
			var [seconds,nanos] = ToParts();
			return json{ {"seconds", seconds}, {"nanos", nanos} };
		}
		α ToProto()Ι->google::protobuf::Timestamp{
			var [seconds,nanos] = ToParts();
			google::protobuf::Timestamp t;
			t.set_seconds( seconds );
			t.set_nanos( nanos );
			return t;
		}
		α ToDuration()Ι->google::protobuf::Duration{
			var ts = ToProto();
			google::protobuf::Duration d; d.set_seconds( ts.seconds() ); d.set_nanos( ts.nanos() );
			return d;
		}

	private:
		α ToParts()Ι->tuple<_int,int>{ 
			var dts = UA_DateTime_toStruct( _dateTime );
			_int seconds = Clock::to_time_t( Chrono::ToTimePoint((int16)dts.year, (int8)dts.month, (int8)dts.day, (int8)dts.hour, (int8)dts.min, (int8)dts.sec) );
			int nanos = dts.milliSec*TimeSpan::MicrosPerMilli*TimeSpan::NanosPerMicro+dts.microSec*TimeSpan::NanosPerMicro+dts.nanoSec;
			return make_tuple( seconds, nanos );
		}
		UA_DateTime _dateTime;
	};
}
#undef var
#endif