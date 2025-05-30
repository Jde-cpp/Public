#pragma once
#ifndef LOG_TAGS_H
#define LOG_TAGS_H
#include <span>
namespace Jde{
	#define Φ Γ auto
	//struct LogTag{ string Id; ELogLevel Level{ELogLevel::NoLog}; };//loadLibrary dlls may disappear, so need string vs. sv
	enum class ELogTags : uint{
		None 					= 0x0,
		Access				= 1ul << 0,
		App 					= 1ul << 1,
		Cache	 	  		= 1ul << 2,
		Client  			= 1ul << 3,
		Crypto				= 1ul << 4,
		DBDriver			= 1ul << 5,
		Exception			= 1ul << 6,
		ExternalLogger= 1ul << 7,
		Http  				= 1ul << 8,
		IO			 			= 1ul << 9,
		Locks	 	  		= 1ul << 10,
		Parsing 			= 1ul << 11,
		Pedantic 			= 1ul << 12,
		QL			 			= 1ul << 13,
		Read	  			= 1ul << 14,
		Scheduler	 		= 1ul << 15,
		Server  			= 1ul << 16,
		Sessions  		= 1ul << 17,
		Settings 			= 1ul << 18,
		Shutdown 			= 1ul << 19,
		Socket  			= 1ul << 20,
		Sql						= 1ul << 21,
		Startup 			= 1ul << 22,
		Subscription 	= 1ul << 23,
		Test					= 1ul << 24,
		Threads				= 1ul << 25,
		Write		 			= 1ul << 26,
		EndOfList			= 1ul << 27,

		HttpClientRead	  = Http | Client | Read,
		HttpClientWrite		= Http | Client | Write,
		HttpClientSessions= Http | Client | Sessions,
		HttpServerRead	  = Http | Server | Read,
		HttpServerWrite		= Http | Server | Write,
		SocketClientRead  = Socket | Client | Read,
		SocketClientWrite	= Socket | Client | Write,
		SocketServerRead  = Socket | Server | Read,
		SocketServerWrite	= Socket | Server | Write
	};
	constexpr ELogTags DefaultTag=ELogTags::App;
	Φ ShouldTrace( ELogTags tags )ι->bool;
	Φ FileMinLevel( ELogTags tags )ι->ELogLevel;
	α MinLevel( ELogTags tags )ι->ELogLevel;
	α Min( ELogLevel a, ELogLevel b )ι->ELogLevel;
	α Min( ELogTags tags, const concurrent_flat_map<ELogTags,ELogLevel>& tagSettings )ι->optional<ELogLevel>;
	α ToString( ELogTags tags )ι->string;
	Φ ToLogTags( sv name )ι->ELogTags;
	Φ TagFromString( function<optional<ELogTags>(sv)> tagFromString )ι->void;
	Φ TagToString( function<string(ELogTags)> tagToString )ι->void;

namespace Logging{
	α TagSettings( string name, str path )ι->concurrent_flat_map<ELogTags,ELogLevel>;
	α AddFileTags()ι->void;
	Φ SetTag( sv tag, ELogLevel l=ELogLevel::Debug, bool file=true )ι->void;
	//α SetTag(sv tag, vector<ELogTags>& existing, ELogLevel configLevel)ι->string;
	α AddTags( concurrent_flat_map<ELogTags, ELogLevel>& sinkTags, sv path)ι->void;
	struct ExternalMessage;
	α ShouldLog( const ExternalMessage& m )ι->bool;
}}
#undef Φ
#endif