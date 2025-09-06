#pragma once
#include <stacktrace>
#ifndef _MSC_VER
	#include <signal.h>
#endif
#include "../process.h"
#include "log.h"
#include "SpdLog.h"

#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&
#define let const auto
#define Φ Γ auto

#ifdef NDEBUG
	#define BREAK
	#define BREAK_IF(x)
#else
	#ifdef _MSC_VER
		#define BREAK DebugBreak();
	#else
		#define BREAK if( Logging::CanBreak() ){ ::raise( 5/*SIGTRAP*/ ); }
	#endif
	#define BREAK_IF(x) if( x ){ BREAK; }
#endif

namespace Jde::Logging{
	Φ BreakLevel()ι->ELogLevel;
	Φ CanBreak()ι->bool;
	α LogException( const IException& e )ι->void;
	α Log( const Entry& entry )ι->void;

	template<ELogLevel TLevel, typename... Args>
	struct Logger{
		using enum ELogLevel;
		explicit Logger( ELogTags tags, FormatString&& m, ARGS... args, const spdlog::source_loc& sl )ι;
		explicit Logger( ELogTags tags, FormatString&& m, ARGS... args, SL sl )ι: Logger( tags, FWD(m), FWD(args)..., {sl.file_name(), (int)sl.line(), sl.function_name()} ){}
	};

	template<ELogLevel TLevel, typename... Args>
	α LogStack( ELogTags tags, std::stacktrace::size_type stackTraceIndex, FormatString&& m, ARGS... args )ι->void{
		if( !ShouldLog(TLevel, tags) )//auto fileMinLevel = FileMinLevel(tags); fileMinLevel==ELogLevel::NoLog || fileMinLevel>TLevel
			return;

		if( let stacktrace = std::stacktrace::current(); stacktrace.size() )
			Log( Entry{stacktrace[ std::min(stacktrace.size()-1,stackTraceIndex+1) ], TLevel, tags, FWD(m), FWD(args)...} );
		else
			Logger<TLevel,Args...>{ tags, FWD(m), FWD(args)..., source_location::current() };
	}
}

namespace Jde{
#define LoggerLevel( Level )	\
	template<typename... Args> \
	struct Level : Logging::Logger<ELogLevel::Level,Args...>{ \
    explicit Level(ELogTags tags, FormatString&& m, ARGS... args, SRCE): \
			Logging::Logger<ELogLevel::Level,Args...>{ tags, FWD(m), FWD(args)..., sl } \
		{} \
    explicit Level( SL sl, ELogTags tags, FormatString&& m, ARGS... args ): \
			Logging::Logger<ELogLevel::Level,Args...>{ tags, FWD(m), FWD(args)..., sl } \
		{} \
	}; \
	template<class... Args> \
	Level( ELogTags tags, FormatString&&, ARGS... )->Level<Args...>

	LoggerLevel( Trace );
	LoggerLevel( Debug );
	LoggerLevel( Information );
	LoggerLevel( Warning );
	LoggerLevel( Error );
	LoggerLevel( Critical );
#undef LoggerLevel

#define CMD(Level)	Logging::Logger<ELogLevel::Level,Args...>{ tags, FWD(m), std::forward<const Args>(args)..., sl };
	ψ Log( ELogLevel level, ELogTags tags, const spdlog::source_loc& sl, FormatString&& m, ARGS... args )ι->void{
		switch( level ){
			case ELogLevel::Trace: CMD( Trace ); break;
			case ELogLevel::Debug: CMD( Debug ); break;
			case ELogLevel::Information: CMD( Information ); break;
			case ELogLevel::Warning: CMD( Warning ); break;
			case ELogLevel::Error: CMD( Error ); break;
			case ELogLevel::Critical: CMD( Critical ); break;
		}
	}
#undef CMD
	ψ Log( ELogLevel level, ELogTags tags, const std::source_location& sl, FormatString&& m, ARGS... args )ι->void{
		Log( level, tags, { sl.file_name(), (int)sl.line(), sl.function_name() }, FWD( m ), FWD( args )... );
	}

#define CMD(Level) Logging::LogStack<ELogLevel::Level,Args...>( tags, stackTraceIndex+1, FWD(m), FWD(args)... )
	ψ Log( ELogLevel level, ELogTags tags, std::stacktrace::size_type stackTraceIndex, FormatString&& m, ARGS... args )ι->void{
		switch( level ){
			case ELogLevel::Trace: CMD( Trace ); break;
			case ELogLevel::Debug: CMD( Debug ); break;
			case ELogLevel::Information: CMD( Information ); break;
			case ELogLevel::Warning: CMD( Warning ); break;
			case ELogLevel::Error: CMD( Error ); break;
			case ELogLevel::Critical: CMD( Critical ); break;
		}
	}
#undef CMD
}

namespace Jde::Logging{
	template<ELogLevel TLevel, typename... Args>
	Logger<TLevel,Args...>::Logger( ELogTags tags, FormatString&& m, ARGS... args, const spdlog::source_loc& sl )ι{
		BREAK_IF( m.get().size()==0 );
		if( Process::Finalizing() || !ShouldLog(TLevel, tags) )
			return;
		for( auto& logger : Loggers() ){
			if( !logger->ShouldLog(TLevel, tags) )
				continue;
			try{
				if( auto p = dynamic_cast<SpdLog*>(logger.get()); p )
					p->Write( TLevel, sl, FWD(m), FWD(args)... );
				else
					logger->Write( Entry{sl, TLevel, tags, string{m.get().data(), m.get().size()}, FWD(args)...} );
			}
			catch( const fmt::format_error& e ){
				Jde::Critical{ ELogTags::App, "could not format '{}' cargs: {} error: '{}'", string{m.get().data(), m.get().size()}, sizeof...(args), string{e.what()} };
			}
		}
		BREAK_IF( tags<=ELogTags::Write && TLevel>=BreakLevel() );//don't want to break for opc server.
	}
}

#undef FormatString
#undef ARGS
#undef let
#undef Φ