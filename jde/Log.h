﻿#pragma once
#ifndef LOG_H
#define LOG_H

#include <iostream>
#include "Exports.h"
#include "collections/ToVec.h"
#include "io/Crc.h"
#include "TypeDefs.h"
#ifndef _MSC_VER
	#include <signal.h>
#endif

#define Φ Γ auto

//namespace Jde::IO{ class IncomingMessage; }
namespace Jde{ 
	struct LogTag{ string Id; ELogLevel Level{ELogLevel::NoLog}; };//loadLibrary dlls may disappear, so need string vs. sv
	Φ CanBreak()ι->bool;
}
namespace Jde::Logging{
	Φ BreakLevel()ι->ELogLevel;
	namespace Messages{ struct ServerMessage; }

	enum class EFields : uint16{ None=0, Timestamp=0x1, MessageId=0x2, Message=0x4, Level=0x8, FileId=0x10, File=0x20, FunctionId=0x40, Function=0x80, LineNumber=0x100, UserId=0x200, User=0x400, ThreadId=0x800, Thread=0x1000, VariableCount=0x2000, SessionId=0x4000 };
	constexpr inline EFields operator|(EFields a, EFields b){ return (EFields)( (uint16)a | (uint16)b ); }
	constexpr inline EFields operator&(EFields a, EFields b){ return (EFields)( (uint16)a & (uint16)b ); }
	constexpr inline EFields operator~(EFields a){ return (EFields)( ~(uint16)a ); }
	constexpr inline EFields& operator|=(EFields& a, EFields b){ return a = a | b; }
	inline std::ostream& operator<<( std::ostream& os, const EFields& value ){ os << (uint)value; return os; }

	struct MessageBase{
		using ID=uint32;
		using ThreadID=uint;
		consteval MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId=0, ID fileId=0, ID functionId=0 )ι;
		consteval MessageBase( sv message, const char* file, const char* function, uint_least32_t line )ι;
		consteval MessageBase( const char* file, const char* function, uint_least32_t line )ι;

		EFields Fields{ EFields::None };
		ELogLevel Level;
		ID MessageId{0};
		sv MessageView;
		ID FileId{0};
		const char* File;
		ID FunctionId{0};
		const char* Function;
		uint_least32_t LineNumber;
		ID UserId{0};
		ThreadID ThreadId{0};
		Γ MessageBase( ELogLevel level, sv message, const char* file, const char* function, uint_least32_t line )ι;
	protected:
		explicit Γ MessageBase( ELogLevel level, SL sl )ι;
	};
	
	struct Message /*final*/ : MessageBase{
		Message( const MessageBase& b )ι;
		Message( const Message& x )ι;
		Γ Message( ELogLevel level, string message, SRCE )ι;
		Γ Message( sv Tag, ELogLevel level, string message, SRCE )ι;
		Γ Message( sv Tag, ELogLevel level, string message, char const* file_, char const * function_, boost::uint_least32_t line_ )ι;

		sv Tag;
		up<string> _pMessage;//todo move to protected
	protected:
		string _fileName;
	};

	Φ SetTag( sv tag, ELogLevel l=ELogLevel::Debug, bool file=true )ι->void;
	//α Log( const Logging::MessageBase& messageBase )ι->void;
	ψ Log( ELogLevel level, Logging::MessageBase&& m, Args&&... args )ι->void;
	ψ Log( const Logging::MessageBase& m, const sp<LogTag> tag, bool logServer, bool break_, Args&&... args )ι->void;
	ψ Log( const Logging::MessageBase& m, const sp<LogTag> tag, Args&&... args )ι->void{ Log( m, tag, true, true, args... ); }

	Φ ShouldLogOnce( const Logging::MessageBase& messageBase )ι->bool;
	Φ LogOnce( Logging::MessageBase&& messageBase, const sp<LogTag>& logTag )ι->void;
	ψ LogOnce( const Logging::MessageBase& messageBase, const sp<LogTag>& logTag, Args&&... args )ι->void;
	α LogNoServer( const Logging::MessageBase& messageBase, const sp<LogTag>& tag )ι->void;
	ψ LogNoServer( const Logging::MessageBase& messageBase, const sp<LogTag>& tag, Args&&... args )ι->void;
	Φ LogServer( const Logging::MessageBase& messageBase )ι->void;
	Φ LogServer( const Logging::MessageBase& messageBase, vector<string>& values )ι->void;
	Φ LogServer( Logging::Messages::ServerMessage& message )ι->void;
	Φ LogMemory( const Logging::MessageBase& messageBase )ι->void;
	Φ LogMemory( const Logging::MessageBase& messageBase, vector<string> values )ι->void;
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

#ifdef NDEBUG
	#define BREAK
	#define BREAK_IF(x)
#else
	#ifdef _MSC_VER
		#define BREAK DebugBreak()
	#else
		#define BREAK if( CanBreak() ){ ::raise( 5/*SIGTRAP*/ ); }
	#endif
	#define BREAK_IF(x) if( x ){ BREAK };
#endif


//namespace spdlog{
//#ifdef _MSC_VER
//	namespace level{ enum level_enum; }
//#endif
//}

namespace Jde{
	Ξ Log( ELogLevel sev, string&& x, const sp<LogTag>& tag, SRCE )ι{ Logging::Log( Logging::Message{sev, move(x), sl}, tag ); }
	Ξ Dbg( string x, const sp<LogTag>& tag, SRCE )ι{ Log( ELogLevel::Debug, move(x), tag, sl ); }

   using namespace std::literals;
	Φ HaveLogger()ι->bool;
	Φ ClearMemoryLog()ι->void;
	Φ FindMemoryLog( uint32 messageId )ι->vector<Logging::Messages::ServerMessage>;
	namespace Logging{
		Φ DestroyLogger()->void;
		Φ Initialize()ι->void;

		Φ Tag( sv tag )ι->sp<LogTag>;
		Φ Tag( const std::span<const sv> tags )ι->vector<sp<LogTag>>;
		Φ LogMemory()ι->bool;
		Φ ServerLevel()ι->ELogLevel;
		Φ ClientLevel()ι->ELogLevel;
		Φ Default()ι->spdlog::logger*;
	}

	inline constexpr PortType ServerSinkDefaultPort = 4321;

	namespace Logging{
		namespace Proto{class Status;}
		α StartTime()ι->TimePoint;
		Φ SetStatus( const vector<string>& values )ι->void;
		α SetLogLevel( ELogLevel client, ELogLevel server )ι->void;
		α GetStatus()ι->up<Proto::Status>;
	}
#define var const auto
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
		//TODO just use format vs vformat catch fmt::v8::format_error in vformat version
		//assert( m.Level<=ELogLevel::None );
		if( m.Level<tag->Level || m.Level==ELogLevel::NoLog || tag->Level==ELogLevel::NoLog )
			return;
		try{
			if( auto p = Default(); p ){
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
		logServer = logServer && ServerLevel()!=ELogLevel::NoLog && ServerLevel()<=m.Level;
		if( logServer || LogMemory() ){
			vector<string> values; values.reserve( sizeof...(args) );
			ToVec::Append( values, args... );
			if( LogMemory() )
				LogMemory( m, values );
			if( logServer )
				LogServer( m, values );
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
#pragma region MessageBase
	consteval MessageBase::MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId, ID fileId, ID functionId )ι:
		Level{level},
		MessageId{ messageId ? messageId : IO::Crc::Calc32(message.substr(0, 100)) },//{},
		MessageView{message},
		FileId{ fileId ? fileId : IO::Crc::Calc32(file) },
		File{file},
		FunctionId{ functionId ? functionId : IO::Crc::Calc32(function) },
		Function{function},
		LineNumber{line}{
		if( level!=ELogLevel::Trace )
			Fields |= EFields::Level;
		if( message.size() )
			Fields |= EFields::Message | EFields::MessageId;
		if( File[0]!='\0' )
			Fields |= EFields::File | EFields::FileId;
		if( Function[0]!='\0' )
			Fields |= EFields::Function | EFields::FunctionId;
		if( LineNumber )
			Fields |= EFields::LineNumber;
	}
	consteval MessageBase::MessageBase( sv message, const char* file, const char* function, uint_least32_t line )ι:
		MessageBase{ message, ELogLevel::Trace, file, function, line }
	{}

	consteval MessageBase::MessageBase( const char* file, const char* function, uint_least32_t line )ι:
		MessageBase( {}, file, function, line )
	{}
}
#pragma endregion

#undef SOURCE
#undef var
#undef Φ
#endif