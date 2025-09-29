#pragma once
#ifndef LOGGER
#define LOGGER
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
	Φ Log( const Entry& entry )ι->void;

	ψ Log( ELogLevel level, ELogTags tags, SL sl, FormatString&& m, ARGS... args )ι->void;
//	ψ Log( ELogTags tags, SL sl, FormatString&& m, ARGS... args )ι->void;

	ψ LogStack( ELogLevel level, ELogTags tags, std::stacktrace::size_type stackTraceIndex, FormatString&& m, ARGS... args )ι->void{
		if( !ShouldLog(level, tags) )
			return;

		if( let stacktrace = std::stacktrace::current(); stacktrace.size() )
			Log( Entry{stacktrace[ std::min(stacktrace.size()-1,stackTraceIndex+1) ], level, tags, FWD(m), FWD(args)...} );
		else
			Log( level, tags, SRCE_CUR, FWD(m), FWD(args)... );
	}

	Φ MarkLogged( StringMd5 id )ι->bool;
	template<typename... Args>
	α LogOnce( SL sl, ELogTags tags, FormatString&& m, ARGS... args )ι->void{
		if( MarkLogged(Entry::GenerateId(sv{m.get().data(), m.get().size()})) )
			Log( ELogLevel::Information, tags, sl, FWD(m), FWD(args)... );
	}
}

namespace Jde{
/*
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
*/
}

namespace Jde::Logging{
	ψ Logging::Log( ELogLevel level, ELogTags tags, SL sl, FormatString&& m, ARGS... args )ι->void{
		BREAK_IF( m.get().size()==0 );
		if( Process::Finalizing() || !ShouldLog(level, tags) )
			return;
		for( auto& logger : Loggers() ){
			if( !logger->ShouldLog(level, tags) )
				continue;
			try{
				if( auto p = dynamic_cast<SpdLog*>(logger.get()); p )
					p->Write( level, sl, FWD(m), FWD(args)... );
				else
					logger->Write( Entry{sl, level, tags, string{m.get().data(), m.get().size()}, FWD(args)...} );
			}
			catch( const fmt::format_error& e ){
				Log( ELogLevel::Critical, ELogTags::App, SRCE_CUR, "could not format '{}' cargs: {} error: '{}'", string{m.get().data(), m.get().size()}, sizeof...(args), string{e.what()} );
			}
		}
		BREAK_IF( tags<=ELogTags::Write && level>=BreakLevel() );//don't want to break for opc server.
	}
}

#undef FormatString
#undef ARGS
#undef let
#undef Φ
#endif