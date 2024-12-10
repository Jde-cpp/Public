#pragma once
#ifndef LOG_H
#define LOG_H

#include <iostream>
#include "../exports.h"
#include "../collections/Vector.h"
#include "../io/crc.h"
#include "../process.h"
#include "logTags.h"
#include "Message.h"
#include "IExternalLogger.h"
#ifndef _MSC_VER
	#include <signal.h>
#endif

#define Φ Γ auto
namespace Jde{
	α ToString( ELogLevel l )ι->string;
	α ToLogLevel( sv op )ι->ELogLevel;
	α LogLevelStrings()ι->const std::array<sv,7>;
}
namespace Jde::Logging{
	ψ Log( ELogLevel level, Logging::MessageBase&& m, Args&&... args )ι->void;
	ψ Log( const Logging::MessageBase& m, const sp<LogTag> tag, bool logServer, bool break_, Args&&... args )ι->void;
	ψ Log( const Logging::MessageBase& m, const sp<LogTag> tag, Args&&... args )ι->void{ Log( m, tag, true, true, args... ); }

	Φ ShouldLogOnce( const Logging::MessageBase& messageBase )ι->bool;
	Φ LogOnce( Logging::MessageBase&& messageBase, const sp<LogTag>& logTag )ι->void;
	ψ LogOnce( const Logging::MessageBase& messageBase, const sp<LogTag>& logTag, Args&&... args )ι->void;
	α LogNoServer( const Logging::MessageBase& messageBase, const sp<LogTag>& tag )ι->void;
	ψ LogNoServer( const Logging::MessageBase& messageBase, const sp<LogTag>& tag, Args&&... args )ι->void;
	Φ LogMemory( const Logging::MessageBase& messageBase )ι->void;
	Φ LogMemory( Logging::Message&& m, vector<string> values )ι->void;
	ψ Tag( sv tag, Logging::MessageBase m, Args&&... args )ι->void;
}

#define MY_FILE __FILE__

#define CRITICAL(message,...) CRITICALT( _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define CRITICALT(logTag, message,...) LOG( ELogLevel::Critical, logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define ERR( message, ... ) ERRT( _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define ERRT( logTag, message, ... ) LOG( ELogLevel::Error, logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define ERRX(message,...) LOGX( ELogLevel::Error, _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define WARN(message,...) WARNT( _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define WARNT(logTag, message, ...) LOG( ELogLevel::Warning, logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define WARN_IF(predicate, message,...) if( predicate ) WARN( message __VA_OPT__(,) __VA_ARGS__ )
#define INFO(message,...) INFOT( _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define INFOT(logTag, message,...) LOG( ELogLevel::Information, logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define DBG(message,...) DBGT( _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define DBGSL(message,...) LOGSL( ELogLevel::Debug, _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define DBGT(logTag, message,...) LOG( ELogLevel::Debug, logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define DBGX(message,...) LOGX( ELogLevel::Debug, _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define DBG_IF(predicate,message,...) if( predicate ) DBG( message __VA_OPT__(,) __VA_ARGS__ )
#define RETURN_IF( predicate, severity, message, ... ) if( predicate ){ LOG(severity, _logTag, message __VA_OPT__(,) __VA_ARGS__); return; }
#define TRACE(message,...) TRACET( _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define TRACET(logTag, message,...) LOG( ELogLevel::Trace, logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define TRACEX(message,...) LOGX( ELogLevel::Trace, _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define TRACESL(message,...) LOGSL( ELogLevel::Trace, _logTag, message __VA_OPT__(,) __VA_ARGS__ )

#define LOG( severity, logTag, message, ...) Logging::Log( Logging::MessageBase(message, severity, MY_FILE, __func__, __LINE__), logTag __VA_OPT__(,) __VA_ARGS__ )
#define LOGX( severity, logTag, message, ...) Logging::LogNoServer( Logging::MessageBase(message, severity, MY_FILE, __func__, __LINE__), logTag __VA_OPT__(,) __VA_ARGS__ )
#define LOGSL( severity, logTag, message, ...) Logging::Log( Logging::Message(severity, message, sl), logTag __VA_OPT__(,) __VA_ARGS__ )
#define LOG_ONCE( severity, logTag, message, ... ) Logging::LogOnce( Logging::MessageBase( message, severity, MY_FILE, __func__, __LINE__), logTag  __VA_OPT__(,) __VA_ARGS__ )

#define LOG_IF( predicate, severity, message,... ) LOG_IFT( predicate, severity, _logTag, message __VA_OPT__(,) __VA_ARGS__ )
#define LOG_IFT( predicate, severity, logTag, message,... ) if( predicate ){ LOG( severity, logTag, message __VA_OPT__(,) __VA_ARGS__ ); }
#define ERR_IF( predicate, message, ... ) LOG_IFL( predicate, ELogLevel::Error, message, __VA_ARGS__ )
#define LOG_MEMORY( tag, severity, message, ... ) LogMemoryDetail( Logging::Message{tag, severity, message} __VA_OPT__(,) __VA_ARGS__ );

namespace Jde{
	Ξ Log( ELogLevel sev, string&& x, const sp<LogTag>& tag, SRCE )ι{ Logging::Log( Logging::Message{sev, move(x), sl}, tag ); }
	Ξ Dbg( string x, const sp<LogTag>& tag, SRCE )ι{ Log( ELogLevel::Debug, move(x), tag, sl ); }

	template<ELogLevel TLevel, typename... Args>
	struct Logger{
    explicit Logger(ELogTags logTags, fmt::format_string<Args&...> m, const Args&... args, std::source_location source = std::source_location::current()) {
			Logging::Log( Logging::MessageBase{ TLevel, sv{m.get().data(), m.get().size()}, source.file_name(), source.function_name(), source.line() }, logTags, std::forward<const Args>(args)... );
		}
	};

	template<typename... Args>
	struct Trace_ : Logger<ELogLevel::Trace,Args...>{
    explicit Trace_(ELogTags tags, fmt::format_string<Args&...> m, const Args&... args, std::source_location source = std::source_location::current()):
			Logger<ELogLevel::Trace,Args...>{ tags, std::forward<fmt::format_string<Args&...>>(m), std::forward<const Args>(args)..., source }
		{}
	};
#define LoggerLevel( Level )	\
	template<typename... Args> \
	struct Level : Logger<ELogLevel::Level,Args...>{ \
    explicit Level( ELogTags tags, fmt::format_string<Args&...> m, const Args&... args, SRCE ): \
		Logger<ELogLevel::Level,Args...>{ tags, std::forward<fmt::format_string<Args&...>>(m), std::forward<const Args>(args)..., sl } \
		{} \
	}; \
	template<class... Args> \
	Level( ELogTags tags, const fmt::format_string<Args&...>, const Args&... )->Level<Args...>

	template<class... Args>
	Trace_( ELogTags tags, const fmt::format_string<Args&...>, const Args&... )->Trace_<Args...>;
	// LoggerLevel( Debug );
	// LoggerLevel( Information );
	// LoggerLevel( Warning );
	// LoggerLevel( Error );
	// LoggerLevel( Critical );
	/*
	template<class... Args>
	debug( ELogTags logTags, const fmt::format_string<Args const&...>, const Args&... )->Logger<ELogLevel::Debug,Args...>;

	template<class... Args>
	critical( ELogTags logTags, const fmt::format_string<Args const&...>, const Args&... )->Logger<ELogLevel::Critical,Args...>;
*/
   using namespace std::literals;
	Φ ClearMemoryLog()ι->void;
	Φ FindMemoryLog( uint32 messageId )ι->vector<Logging::ExternalMessage>;
	Φ FindMemoryLog( function<bool(const Logging::ExternalMessage&)> f )ι->vector<Logging::ExternalMessage>;
	//Φ FindMemoryLog( str tag, uint code )ι->vector<Logging::ExternalMessage>;
	namespace Logging{
		Φ DestroyLogger()->void;
		Φ Initialize()ι->void;

		Φ ClientMinLevel()ι->ELogLevel;
		//Φ Default()ι->spdlog::logger*;

//	inline constexpr PortType ServerSinkDefaultPort = 4321;
		namespace Proto{class Status;}
		Φ StartTime()ι->TimePoint;
		Φ SetStatus( const vector<string>& values )ι->void;
		α SetLogLevel( ELogLevel client, ELogLevel server )ι->void;
		α GetStatus()ι->up<Proto::Status>;
	}
#define let const auto
	Ξ FileName( const char* file_ )->string{
//#ifdef NDEBUG
		return file_;
/*#else
		string file{ file_ };
		if( file.starts_with('~') )
			return file;

#ifdef _MSC_VER
		const string homeDir{ file };
#else
		uint start = 0;
		for( uint i=0; i<3 && (start = file.find( '/', start ))!=string::npos; ++i, ++start );

		auto homeDir = start==string::npos ? string{file} : '~'+string{ file.substr(start-1) };
		for( uint i{string::npos}; (i=homeDir.find("/./", 0, 3))!=string::npos; ++i )//{string::npos}:  -W sometimes-uninitialized
			homeDir.replace( i, 3, 1, '/' );

		for( uint end; (end=homeDir.find("/..", 0))!=string::npos; )
		{
			start = homeDir.substr(0,end).find_last_of( '/' ); if( start==string::npos ) break;
			homeDir = homeDir.substr( 0, start )+homeDir.substr( end+3 );
		}
#endif
		return homeDir;
#endif*/
	}
#define SOURCE spdlog::source_loc{ FileName(m.File).c_str(), (int)m.LineNumber, m.Function }
	ψ Logging::Log( ELogLevel level, Logging::MessageBase&& m, Args&&... args )ι->void{
		m.Level = level;
		Log( move(m), args... );
	}
#pragma warning(push)
#pragma warning( disable : 4100)
	ψ Logging::Log( const Logging::MessageBase& m, const sp<LogTag> tag, bool logServer, bool break_, Args&&... args )ι->void{
		ASSERT( !Process::Finalizing() );
		//TODO just use format vs vformat catch fmt::v8::format_error in vformat version
		//assert( m.Level<=ELogLevel::None );
		let tagLevel = FileMinLevel( ToLogTags(tag->Id) );
		if( m.Level<tagLevel || m.Level==ELogLevel::NoLog || tagLevel==ELogLevel::NoLog )
			return;
		try{
			if( auto p = Default(); p ){
				if( m.MessageView.empty() )
					BREAK
				if constexpr( sizeof...(args)>0 )
					p->log( SOURCE, (spdlog::level::level_enum)m.Level, fmt::vformat(std::locale(""), m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
				else
					p->log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
			}
			else{
				if( m.Level>=ELogLevel::Error )
					std::cerr << m.MessageView << std::endl;
				else
					std::cout << m.MessageView << std::endl;
			}

			BREAK_IF( break_ && m.Level>=BreakLevel() );
		}
		catch( const fmt::format_error& e ){
			MessageBase m2{ ELogLevel::Critical, "could not format '{}' cargs='{}' - '{}'", m.File, m.Function, m.LineNumber };
			Log( m2, tag, m.MessageView, sizeof...(args), e.what() );
		}
		logServer = logServer && !(ToLogTags(tag->Id) & ELogTags::ExternalLogger);
		if( logServer || LogMemory() ){
			vector<string> values; values.reserve( sizeof...(args) );
			ToVec::Append( values, args... );
			if( LogMemory() )
				LogMemory( m, values );
			if( logServer )
				External::Log( m, values );
		}
	}
#pragma warning(pop)
	ψ Logging::LogOnce( const Logging::MessageBase& m, const sp<LogTag>& logTag, Args&&... args )ι->void{
		if( ShouldLogOnce(m) )
			Log( m, logTag, args... );
	}

	Ξ Logging::LogNoServer( const Logging::MessageBase& m, const sp<LogTag>& tag )ι->void{
		Log( m, tag, false );
	}

	ψ Logging::LogNoServer( const Logging::MessageBase& m, const sp<LogTag>& tag, Args&&... args )ι->void{
		Log( m, tag, false, args... );
	}
}

namespace Jde::Logging{
	ψ LogMemoryDetail( Logging::Message&& m, Args&&... args )ι->void{
		vector<string> values; values.reserve( sizeof...(args) );
		ToVec::Append( values, args... );
		LogMemory( move(m), move(values) );
	}
}
#undef SOURCE
#undef let
#undef Φ
#endif