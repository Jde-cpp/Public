#pragma once
#ifndef LOG_TAGS_H
#define LOG_TAGS_H

namespace Jde{
	#define Φ Γ auto
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

	namespace Logging{
		struct ILogger;
		α UpdateCumulative( const vector<up<Logging::ILogger>>& loggers )ι->void;
	}
	struct Γ LogTags{
		LogTags( jobject o )ι;
		LogTags( ELogLevel defaultLevel=ELogLevel::Information ):_minLevel{defaultLevel},_defaultLevel{defaultLevel}{}
		β Name()Ι->string{ return "Cumulative"; }
		β MinLevel()Ι->ELogLevel{ return _minLevel; }
		β MinLevel( ELogTags tags )Ι->ELogLevel;
		β SetMinLevel( ELogLevel level )ι->void{ _minLevel = level; }
		α SetLevel( ELogTags tags, ELogLevel level )ι->void;
		β ShouldLog( ELogLevel level, ELogTags tags )Ι->bool;
		β ToString()ι->string;
	protected:
		concurrent_flat_map<ELogTags,ELogLevel> ConfiguredTags;
		mutable concurrent_flat_map<ELogTags,ELogLevel> ExtrapolatedTags;
		ELogLevel	_minLevel;
		ELogLevel _defaultLevel;
		friend α Logging::UpdateCumulative( const vector<up<Logging::ILogger>>& loggers )ι->void;
	};

	constexpr ELogTags DefaultTag=ELogTags::App;
	Φ ShouldTrace( ELogTags tags )ι->bool;
	Φ ToString( ELogTags tags )ι->string;
	Φ ToArray( ELogTags tags )ι->jarray;
	Φ ToLogTags( sv name )ι->ELogTags;
namespace Logging{
	struct ITagParser{
		β ToTag( str tagName )Ι->ELogTags=0;
		β ToString( ELogTags tags )Ι->string=0;
	};
	Φ AddTagParser( up<ITagParser>&& tagParser )ι->void;

	Φ ShouldLog( ELogLevel level, ELogTags tags )ι->bool;
}}
#undef Φ
#endif