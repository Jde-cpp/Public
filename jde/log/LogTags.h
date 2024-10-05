#pragma once
#ifndef LOG_TAGS_H
#define LOG_TAGS_H
namespace Jde{
	#define Φ Γ auto
	struct LogTag{ string Id; ELogLevel Level{ELogLevel::NoLog}; };//loadLibrary dlls may disappear, so need string vs. sv
	enum class ELogTags : uint{
		None 					= 0x0,
		App 					= 1ul << 0,
		Cache	 	  		= 1ul << 1,
		Client  			= 1ul << 2,
		Exception			= 1ul << 3,
		ExternalLogger= 1ul << 4,
		Http  				= 1ul << 5,
		IO			 			= 1ul << 6,
		Locks	 	  		= 1ul << 7,
		Parsing 			= 1ul << 8,
		Pedantic 			= 1ul << 9,
		QL			 			= 1ul << 10,
		Read	  			= 1ul << 11,
		Scheduler	 		= 1ul << 12,
		Server  			= 1ul << 13,
		Sessions  		= 1ul << 14,
		Settings 			= 1ul << 15,
		Shutdown 			= 1ul << 16,
		Socket  			= 1ul << 17,
		Sql						= 1ul << 18,
		Startup 			= 1ul << 19,
		Subscription 	= 1ul << 20,
		Test					= 1ul << 21,
		Threads				= 1ul << 22,
		UM						= 1ul << 23,
		Write		 			= 1ul << 24,

		HttpClientRead	  = Http | Client | Read,
		HttpClientWrite		= Http | Client | Write,
		HttpServerRead	  = Http | Server | Read,
		HttpServerWrite		= Http | Server | Write,
		SocketClientRead  = Socket | Client | Read,
		SocketClientWrite	= Socket | Client | Write,
		SocketServerRead  = Socket | Server | Read,
		SocketServerWrite	= Socket | Server | Write
	};
	constexpr ELogTags DefaultTag=ELogTags::App;
	Φ ShouldTrace( sp<LogTag> pTag )ι->bool;
	Φ ShouldTrace( ELogTags tags )ι->bool;
	Φ FileMinLevel( ELogTags tags )ι->ELogLevel;
	α MinLevel( ELogTags tags )ι->ELogLevel;
	α Min( ELogLevel a, ELogLevel b )ι->ELogLevel;
	α Min( ELogTags tags, const concurrent_flat_map<ELogTags,ELogLevel>& tagSettings )ι->optional<ELogLevel>;
	α ToString( ELogTags tags )ι->string;
	Φ ToLogTags( str name )ι->ELogTags;
	Φ TagParser( function<optional<ELogTags>(sv)> parser )ι->void;

namespace Logging{
	α AddTags( vector<LogTag>& sinkTags, sv path )ι->void;
	α TagSettings( string name, str path )ι->concurrent_flat_map<ELogTags,ELogLevel>;
	α AddFileTags()ι->void;
	Φ SetTag( sv tag, ELogLevel l=ELogLevel::Debug, bool file=true )ι->void;
	struct ExternalMessage;
	α ShouldLog( const ExternalMessage& m )ι->bool;
	Φ Tag( sv tag )ι->sp<LogTag>;
	Φ Tag( ELogTags tag )ι->sp<LogTag>;
	Φ Tag( const std::span<const sv> tags )ι->vector<sp<LogTag>>;
}}
#undef Φ
#endif