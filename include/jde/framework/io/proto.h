#pragma once
#ifndef PROTO_UTILITIES_H
#define PROTO_UTILITIES_H
#pragma warning(push)
#pragma warning( disable : 4127 )
#pragma warning( disable : 5054 )
#include <google/protobuf/message.h>
#include <google/protobuf/timestamp.pb.h>
#pragma warning(pop)
#include <jde/framework/io/file.h>

#define let const auto
namespace Jde::Proto{
	Ŧ Load( const fs::path& path, SRCE )ε->up<T>;
	Ŧ TryLoad( const fs::path& path, SRCE )ι->up<T>;
	Ŧ Load( const fs::path& path, T& p, SRCE )ε->void;

	Ŧ Deserialize( const vector<char>& data )ε->up<T>;
	Ŧ Deserialize( const google::protobuf::uint8* p, int size )ε->T;
	Ŧ Deserialize( string&& x )ε->T;
	Ŧ FromVector( vector<T>&& x )ι->google::protobuf::RepeatedPtrField<T>;
	Ŧ ToVector( google::protobuf::RepeatedPtrField<T>&& x )ι->vector<T>;
	Ŧ ToVector( const google::protobuf::RepeatedField<T>& x )ι->vector<T>;

	α Save( const google::protobuf::MessageLite& msg, fs::path path, SL )ε->void;
	α ToString( const google::protobuf::MessageLite& msg )ι->string;
	α SizePrefixed( const google::protobuf::MessageLite& m )ι->vector<byte>;
	α ToTimestamp( TimePoint t )ι->google::protobuf::Timestamp;
	Ξ ToBytes( const uuid& g )ι->sv{ return sv{ (char*)g.data(), 16 }; }
	Ξ ToGuid( string g )ι->uuid{ uuid y; memcpy( &y, g.data(), std::min(sizeof(uuid), g.size())); return y; }
	α ToTimePoint( google::protobuf::Timestamp t )ι->TimePoint;

	namespace Internal{
		Ŧ Deserialize( const google::protobuf::uint8* p, int size, T& proto )ε->void{
			google::protobuf::io::CodedInputStream input{ p, size };
			THROW_IF( !proto.MergePartialFromCodedStream(&input), "MergePartialFromCodedStream returned false." );
		}
	}
}


namespace Jde{
	Ξ Proto::ToString( const google::protobuf::MessageLite& msg )ι->string{
		string output;
		msg.SerializeToString( &output );
		return output;
	}

	Ξ Proto::SizePrefixed( const google::protobuf::MessageLite& m )ι->vector<byte>{
		const uint32_t length = (uint32_t)m.ByteSizeLong();
		let size = length+4;
		vector<byte> data( size ); data.reserve( size );
		auto dest = data.data();
		const char* pLength = reinterpret_cast<const char*>( &length )+3;
		for( auto i=0; i<4; ++i )
			*dest++ = (byte)*pLength--;
		if( let success = m.SerializeToArray( dest, (int)length ); !success )
			CRITICALT( Jde::ELogTags::IO, "Could not serialize to an array:{:x}", success );
		return data;
	}
	Ξ Proto::Save( const google::protobuf::MessageLite& msg, fs::path path, SRCE )ε->void{
		string content;
		msg.SerializeToString( &content );
		IO::Save( move(path), content, sl );
	}

	Ŧ Proto::Deserialize( const vector<char>& data )ε->up<T>{
		auto p = mu<T>();
		Internal::Deserialize<T>( (google::protobuf::uint8*)data.data(), (uint32)data.size(), *p );
		return p;
	}
	Ŧ Proto::Deserialize( const google::protobuf::uint8* p, int size )ε->T{
		T y;
		Internal::Deserialize<T>( p, size, y );
		return y;
	}
	Ŧ Proto::Deserialize( string&& x )ε->T{
		T y;
		Internal::Deserialize<T>( (google::protobuf::uint8*)x.data(), (int)x.size(), y );
		x.clear();
		return y;
	}

	Ŧ Proto::Load( const fs::path& path, T& proto, SL sl )ε->void{
		vector<char> bytes;
		try{
			bytes = IO::LoadBinary( path, sl );
		}
		catch( fs::filesystem_error& e ){
			throw IOException( move(e) );
		}
		if( !bytes.size() ){
			fs::remove( path );
			throw IOException{ path, "has 0 bytes. Removed", sl };
		}
		Internal::Deserialize( (google::protobuf::uint8*)bytes.data(), (uint32)bytes.size(), proto );
	}

	Ŧ Proto::Load( const fs::path& path, SL sl )ε->up<T>{
		auto p = mu<T>();
		Load( path, *p, sl );
		return p;
	}

	Ŧ Proto::TryLoad( const fs::path& path, SL sl )ι->up<T>{
		up<T> pValue{};
		if( fs::exists(path) ){
			try{
				pValue = Load<T>( path, sl );
			}
			catch( const IException& )
			{}
		}
		return pValue;
	}

	Ŧ Proto::FromVector( vector<T>&& x )ι->google::protobuf::RepeatedPtrField<T>{
		google::protobuf::RepeatedPtrField<T> y;
		for( auto& item : x )
			*y.Add() = std::move(item);
		x.clear();
		return y;
	}
	Ŧ Proto::ToVector( google::protobuf::RepeatedPtrField<T>&& x )ι->vector<T>{
		vector<T> y; y.reserve( x.size() );
		for_each( x, [&y]( auto& item ){ y.push_back(move(item)); } );
		return y;
	}
	Ŧ Proto::ToVector( const google::protobuf::RepeatedField<T>& field )ι->vector<T>{
		return vector<T>{ std::begin(field), std::end(field) };
	}

	Ξ Proto::ToTimestamp( TimePoint t )ι->google::protobuf::Timestamp{
		google::protobuf::Timestamp proto;
		let seconds = Clock::to_time_t( t );
		let nanos = std::chrono::duration_cast<std::chrono::nanoseconds>( t-TimePoint{} )-std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::seconds(seconds) );
		proto.set_seconds( seconds );
		proto.set_nanos( (int)nanos.count() );
		return proto;
	}
	Ξ Proto::ToTimePoint( google::protobuf::Timestamp t )ι->TimePoint{
		google::protobuf::Timestamp proto;
#ifdef _MSC_VER
		Clock::duration duration = duration_cast<Clock::duration>( std::chrono::nanoseconds( t.seconds() ) );
		return Clock::from_time_t( t.seconds() ) + duration;
#else
		return Clock::from_time_t( t.seconds() )+std::chrono::nanoseconds( t.nanos() );
#endif
	}
}
#undef let
#endif