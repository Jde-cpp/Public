#pragma once
#include <array>
#include <memory>
#include <iostream> //TODO remove

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>

#include <string_view>
#include <shared_mutex>
#include <sstream>
#include "./Exports.h"
#include "collections/ToVec.h"
#include "io/Crc.h"
#include "TypeDefs.h"
#ifndef NDEBUG
	#ifndef _MSC_VER
		#include "signal.h"
	#endif
#endif

#define Φ Γ auto

namespace Jde::IO{ class IncomingMessage; }
namespace Jde::Logging
{
#ifndef NDEBUG
	Φ BreakLevel()noexcept->ELogLevel;
#endif
	namespace Messages{ struct ServerMessage; }
#pragma region EFields
	enum class EFields : uint16{ None=0, Timestamp=0x1, MessageId=0x2, Message=0x4, Level=0x8, FileId=0x10, File=0x20, FunctionId=0x40, Function=0x80, LineNumber=0x100, UserId=0x200, User=0x400, ThreadId=0x800, Thread=0x1000, VariableCount=0x2000, SessionId=0x4000 };
	constexpr inline EFields operator|(EFields a, EFields b){ return (EFields)( (uint16)a | (uint16)b ); }
	constexpr inline EFields operator&(EFields a, EFields b){ return (EFields)( (uint16)a & (uint16)b ); }
	constexpr inline EFields operator~(EFields a){ return (EFields)( ~(uint16)a ); }
	constexpr inline EFields& operator|=(EFields& a, EFields b){ return a = a | b; }
	inline std::ostream& operator<<( std::ostream& os, const EFields& value ){ os << (uint)value; return os; }
#pragma endregion
	struct MessageBase
	{
		using ID=uint32;
		using ThreadID=uint;
		consteval MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId=0, ID fileId=0, ID functionId=0 )noexcept;
		consteval MessageBase( sv message, const char* file, const char* function, uint_least32_t line )noexcept;
		consteval MessageBase( const char* file, const char* function, uint_least32_t line )noexcept;

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
		Γ MessageBase( ELogLevel level, sv message, const char* file, const char* function, uint_least32_t line )noexcept;
	protected:
		explicit Γ MessageBase( ELogLevel level, SL sl )noexcept;
	};
	struct Message /*final*/ : MessageBase
	{
		Message( const MessageBase& b )noexcept;
		Message( const Message& x )noexcept;
		Γ Message( ELogLevel level, string message, SRCE )noexcept;
		Γ Message( sv Tag, ELogLevel level, string message, SRCE )noexcept;

		sv Tag;
		up<string> _pMessage;//todo move to protected
	protected:
		string _fileName;
	};

	Φ SetTag( sv tag, ELogLevel l=ELogLevel::Debug, bool file=true )noexcept->void;
	α Log( const Logging::MessageBase& messageBase )noexcept->void;
	ψ Log( ELogLevel level, Logging::MessageBase&& m, Args&&... args )noexcept->void;
	ψ Log( const Logging::MessageBase& messageBase, Args&&... args )noexcept->void;

	Φ ShouldLogOnce( const Logging::MessageBase& messageBase )noexcept->bool;
	Φ LogOnce( const Logging::MessageBase& messageBase )noexcept->void;
	ψ LogOnce( const Logging::MessageBase& messageBase, Args&&... args )noexcept->void;
	α LogNoServer( const Logging::MessageBase& messageBase )noexcept->void;
	ψ LogNoServer( const Logging::MessageBase& messageBase, Args&&... args )noexcept->void;
	Φ LogServer( const Logging::MessageBase& messageBase )noexcept->void;
	Φ LogServer( const Logging::MessageBase& messageBase, vector<string>& values )noexcept->void;
	Φ LogServer( Logging::Messages::ServerMessage& message )noexcept->void;
	Φ LogMemory( const Logging::MessageBase& messageBase )noexcept->void;
	Φ LogMemory( const Logging::MessageBase& messageBase, vector<string> values )noexcept->void;
	Φ LogMemory( Logging::Message&& m, vector<string> values )noexcept->void;
	ψ Tag( sv tag, Logging::MessageBase m, Args&&... args )noexcept->void;
}

#define MY_FILE __FILE__

#define CRITICAL(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(message, Jde::ELogLevel::Critical, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Error, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERRX(message,...) Logging::LogNoServer( Logging::MessageBase(message, ELogLevel::Error, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Error, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define WARN(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Warning, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
//#define WARN_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Warning, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define WARN_IF(predicate, message,...) if( predicate ) Logging::Log( Logging::MessageBase(message, ELogLevel::Warning, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define INFO(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Information, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define INFO_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Information, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG_IF(predicate,message,...) if( predicate ) Logging::Log( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define BREAK_IF( predicate, severity, message, ...) if( predicate ){ LOGL(severity, message, __VA_ARGS__); break; }
#define CONTINUE_IF( predicate, message, ...) if( predicate ){ LOG(message __VA_OPT__(,) __VA_ARGS__); continue; }
#define RETURN_IF( predicate, message, ... ) if( predicate ){ LOG( message __VA_OPT__(,) __VA_ARGS__ ); return; }
#define TRACE(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Trace, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )

#define LOGL(severity,message,...) Logging::Log( severity, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGT(severity,message,...) Logging::Log( severity.Level, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGX(message,...) Logging::LogNoServer( Logging::Message(_logLevel.Level, message) __VA_OPT__(,) __VA_ARGS__ )
#define LOG(message,...) Logging::Log( _logLevel.Level, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGS(message,...) Logging::Log( Logging::Message( _logLevel.Level, message ) __VA_OPT__(,) __VA_ARGS__ )

#define LOG_IF(predicate, message,...) if( predicate ) Logging::Log( _logLevel.Level, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOG_IFT(predicate, severity, message,...) if( predicate ) LOGT( severity, message __VA_OPT__(,) __VA_ARGS__ )
#define LOG_IFL(predicate, severity, message,...) if( predicate ) LOGL( severity, message __VA_OPT__(,) __VA_ARGS__ )
#define LOG_TAG_IF(predicate, tag, m,...) if( predicate ) LOGT( Logging::TagLevel(tag), m __VA_OPT__(,) __VA_ARGS__ )
#define ERR_IF(predicate, message,...) LOG_IFL( predicate, ELogLevel::Error, message, __VA_ARGS__ )
#define LOG_MEMORY( tag, severity, message, ... ) LogMemoryDetail( Logging::Message{tag, severity, message} __VA_OPT__(,) __VA_ARGS__ );

#ifdef _MSC_VER
	#define BREAK DebugBreak()
#else
	#define BREAK ::raise( 5/*SIGTRAP*/ )
#endif
#define DEBUG_IF(x) if( x ) BREAK;

namespace spdlog
{
#ifdef _MSC_VER
	namespace level{ enum level_enum : int;}
#endif
}
namespace Jde
{
	Ξ Log( ELogLevel sev, string&& x, SRCE )noexcept{ Logging::Log( Logging::Message{sev, move(x), sl} ); }
	Ξ Dbg( string x, SRCE )noexcept{ Logging::Log( Logging::Message{ELogLevel::Debug, move(x), sl} ); }

   using namespace std::literals;
	Φ HaveLogger()noexcept->bool;
	Φ ClearMemoryLog()noexcept->void;
	Φ FindMemoryLog( uint32 messageId )noexcept->vector<Logging::Messages::ServerMessage>;
	struct LogTag{ sv Id; ELogLevel Level{ELogLevel::NoLog}; /*α Empty()const noexcept{return Id.empty() || Level==ELogLevel::None;}*/ };
	namespace Logging
	{
		Φ DestroyLogger()->void;
		Φ Initialize()noexcept->void;

		Φ TagLevel( sv tag )noexcept->const LogTag&;
		Φ LogMemory()noexcept->bool;
		Φ ServerLevel()noexcept->ELogLevel;
		Φ Default()noexcept->spdlog::logger&;
	}

	constexpr PortType ServerSinkDefaultPort = 4321;

	namespace Logging
	{
		namespace Proto{class Status;}
		α StartTime()noexcept->TimePoint;
		Φ SetStatus( const vector<string>& values )noexcept->void;
		α SetLogLevel( ELogLevel client, ELogLevel server )noexcept->void;
		α GetStatus()noexcept->up<Proto::Status>;
	}
#define var const auto
	Ξ FileName( const char* file_ )->string
	{
#ifdef NDEBUG
		return file_;
#else
		string file{ file_ };
		if( file.starts_with('~') )
			return file;

#ifdef _MSC_VER
		const string homeDir = file.find("\\jde\\")==sv::npos ? string{ file }  : format( "%JDE_DIR%{}", file.substr(file.find("\\jde\\")+4) );
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
#endif
	}
#define SOURCE spdlog::source_loc{ FileName(m.File).c_str(), (int)m.LineNumber, m.Function }
	Ξ Logging::Log( const Logging::MessageBase& m )noexcept->void
	{
		Default().log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
		if( LogMemory() )
			LogMemory( m );
		if( ServerLevel()<=m.Level )
			LogServer( m );
	}

	ψ Logging::Log( ELogLevel level, Logging::MessageBase&& m, Args&&... args )noexcept->void
	{
		m.Level = level;
		Log( move(m), args... );
	}

	ψ Logging::Log( const Logging::MessageBase& m, Args&&... args )noexcept->void
	{//TODO just use format vs vformat catch fmt::v8::format_error in vformat version
		assert( m.Level<=ELogLevel::None );
		if( m.Level>=ELogLevel::None )
			return;
		try
		{
			if constexpr( sizeof...(args)>0 )
				Default().log( SOURCE, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
			else
				Default().log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
#ifndef NDEBUG
			if( m.Level>=BreakLevel() )
				BREAK;
#endif
		}
		catch( const fmt::format_error& )
		{
			Log( MessageBase(ELogLevel::Critical, "could not format {} - {}", m.File, m.Function, m.LineNumber), m.MessageView, sizeof...(args) );
		}
		if( ServerLevel()<=m.Level || LogMemory() )
		{
			vector<string> values; values.reserve( sizeof...(args) );
			ToVec::Append( values, args... );
			if( LogMemory() )
				LogMemory( m, values );
			if( ServerLevel()<=m.Level )
				LogServer( m, values );
		}
	}
	ψ Logging::LogOnce( const Logging::MessageBase& m, Args&&... args )noexcept->void
	{
		if( ShouldLogOnce(m) )
			Log( m, args... );
	}

	Ξ Logging::LogNoServer( const Logging::MessageBase& m )noexcept->void
	{
		Default().log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
	}

	ψ Logging::LogNoServer( const Logging::MessageBase& m, Args&&... args )noexcept->void
	{
		Default().log( SOURCE, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
	}
}

namespace Jde::Logging
{

	ψ LogMemoryDetail( Logging::Message&& m, Args&&... args )noexcept->void
	{
		vector<string> values; values.reserve( sizeof...(args) );
		ToVec::Append( values, args... );
		LogMemory( move(m), move(values) );
	}
#pragma region MessageBase
	consteval MessageBase::MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId, ID fileId, ID functionId )noexcept:
		Level{level},
		MessageId{ messageId ? messageId : IO::Crc::Calc32(message.substr(0, 100)) },//{},
		MessageView{message},
		FileId{ fileId ? fileId : IO::Crc::Calc32(file) },
		File{file},
		FunctionId{ functionId ? functionId : IO::Crc::Calc32(function) },
		Function{function},
		LineNumber{line}
	{
		//ASSERT( file!=nullptr && function!=nullptr );
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
	consteval MessageBase::MessageBase( sv message, const char* file, const char* function, uint_least32_t line )noexcept:
		MessageBase{ message, ELogLevel::Trace, file, function, line }
	{}

	consteval MessageBase::MessageBase( const char* file, const char* function, uint_least32_t line )noexcept:
		MessageBase( {}, file, function, line )
	{}
}
#pragma endregion

#undef SOURCE
#undef var
#undef Φ