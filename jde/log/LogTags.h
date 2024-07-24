#pragma once
namespace Jde{
	#define Φ Γ auto

	struct LogTag{ string Id; ELogLevel Level{ELogLevel::NoLog}; };//loadLibrary dlls may disappear, so need string vs. sv
	enum class ELogTags : uint{
		None 					= 0x0,
		Sql						= 1ul << 0,
		Exception			= 1ul << 1,
		Users					= 1ul << 2,
		Tests					= 1ul << 3,
		App 					= 1ul << 4,
		Settings 			= 1ul << 5,
		IO			 			= 1ul << 6,
		Locks	 	  		= 1ul << 7,
		Alarm	 	  		= 1ul << 8,
		Cache	 	  		= 1ul << 9,
		Sessions  		= 1ul << 10,
		Http  				= 1ul << 11,
		Socket  			= 1ul << 12,
		Client  			= 1ul << 13,
		Server  			= 1ul << 14,
		Read	  			= 1ul << 15,
		Write		 			= 1ul << 16,
		Startup 			= 1ul << 17,
		Shutdown 			= 1ul << 18,
		Subscription 	= 1ul << 19,
		ExternalLogger= 1ul << 20,
		GraphQL 			= 1ul << 21,
		Parsing 			= 1ul << 22,
		HttpClientRead	= Http | Client | Read,
		HttpClientWrite			= Http | Client | Write,
		HttpServerRead	= Http | Server | Read,
		HttpServerWrite			= Http | Server | Write,
		SocketClientRead= Socket | Client | Read,
		SocketClientWrite		= Socket | Client | Write,
		SocketServerRead= Socket | Server | Read,
		SocketServerWrite		= Socket | Server | Write
	};
	constexpr ELogTags DefaultTag=ELogTags::App;
	α ShouldTrace( sp<LogTag> pTag )ι->bool;
	α FileMinLevel( ELogTags tags )ι->ELogLevel;
	α ToString( ELogTags tags )ι->string;
	α ToLogTags( str name )ι->ELogTags;

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