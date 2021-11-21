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

namespace Jde::IO{ class IncomingMessage; }
namespace Jde::Logging
{
	namespace Messages{ struct ServerMessage; }
#pragma region EFields
	enum class EFields : uint16
	{
		None=0,
		Timestamp=0x1,
		MessageId=0x2,
		Message=0x4,
		Level=0x8,
		FileId=0x10,
		File=0x20,
		FunctionId=0x40,
		Function=0x80,
		LineNumber=0x100,
		UserId=0x200,
		User=0x400,
		ThreadId=0x800,
		Thread=0x1000,
		VariableCount=0x2000,
		SessionId=0x4000
	};
	constexpr inline EFields operator|(EFields a, EFields b){ return (EFields)( (uint16)a | (uint16)b ); }
	constexpr inline EFields operator&(EFields a, EFields b){ return (EFields)( (uint16)a & (uint16)b ); }
	constexpr inline EFields operator~(EFields a){ return (EFields)( ~(uint16)a ); }
	constexpr inline EFields& operator|=(EFields& a, EFields b){ return a = a | b; }
	inline std::ostream& operator<<( std::ostream& os, const EFields& value ){ os << (uint)value; return os; }
#pragma endregion
	struct MessageBase
	{
		consteval MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, uint32 messageId=0, uint fileId=0, uint functionId=0 )noexcept;
		consteval MessageBase( sv message, const char* file, const char* function, uint_least32_t line )noexcept;
		consteval MessageBase( const char* file, const char* function, uint_least32_t line )noexcept;

		EFields Fields{ EFields::None };
		ELogLevel Level;
		uint MessageId{0};
		sv MessageView;
		uint FileId{0};
		const char* File;
		uint FunctionId{0};
		const char* Function;
		uint_least32_t LineNumber;
		uint UserId{0};
		uint ThreadId{0};
	protected:
		explicit Γ MessageBase( ELogLevel level, SL sl )noexcept;
	};
	struct Message /*final*/ : MessageBase
	{
		Message( const MessageBase& b )noexcept;
		Message( const Message& x )noexcept:
			MessageBase{ x },
			_pMessage{ x._pMessage ? make_unique<string>(*x._pMessage) : nullptr },
			_fileName{ x._fileName }
		{
			File = _fileName.c_str();
			if( _pMessage )
				MessageView = *_pMessage;
		}
		Γ Message( ELogLevel level, string message, SRCE )noexcept;

		up<string> _pMessage;//todo move to protected
	protected:
		string _fileName;
	};

	Γ α SetTag( sv tag, ELogLevel l=ELogLevel::Debug, bool file=true )noexcept->void;
	α Log( const Logging::MessageBase& messageBase )noexcept->void;
	ψ Log( ELogLevel level, Logging::MessageBase&& m, Args&&... args )noexcept->void;
	ψ Log( const Logging::MessageBase& messageBase, Args&&... args )noexcept->void;

	Γ α ShouldLogOnce( const Logging::MessageBase& messageBase )noexcept->bool;
	Γ α LogOnce( const Logging::MessageBase& messageBase )noexcept->void;
	ψ LogOnce( const Logging::MessageBase& messageBase, Args&&... args )noexcept->void;
	α LogNoServer( const Logging::MessageBase& messageBase )noexcept->void;
	ψ LogNoServer( const Logging::MessageBase& messageBase, Args&&... args )noexcept->void;
	Γ α LogServer( const Logging::MessageBase& messageBase )noexcept->void;
	Γ α LogServer( const Logging::MessageBase& messageBase, vector<string>& values )noexcept->void;
	Γ α LogServer( Logging::Messages::ServerMessage& message )noexcept->void;
	//Γ α LogMemory( const Logging::MessageBase& messageBase )noexcept->void;
	Γ α LogMemory( const Logging::MessageBase& messageBase )noexcept->void;
	Γ α LogMemory( const Logging::MessageBase& messageBase, vector<string> values )noexcept->void;
	Γ α LogMemory( Logging::Message&& m, vector<string> values )noexcept->void;
	//Γ α LogMemory( const Logging::MessageBase& messageBase, vector<string> values )noexcept->void;
	//ψ Tag( sv tag, Logging::MessageBase& m, Args&&... args )noexcept->void;
}

#define MY_FILE __FILE__

#define CRITICAL(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Critical, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Error, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERRX(message,...) Logging::LogNoServer( Logging::MessageBase(message, ELogLevel::Error, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Error, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define WARN(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Warning, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define WARN_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Warning, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define WARN_IF(predicate, message,...) if( predicate ) Logging::Log( Logging::MessageBase(message, ELogLevel::Warning, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define INFO(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Information, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define INFO_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Information, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG_IF(predicate,message,...) if( predicate ) Logging::Log( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define BREAK_IF( predicate, severity, message, ...) if( predicate ){ LOGL(severity, message, __VA_ARGS__); break; }
#define CONTINUE_IF( predicate, message, ...) if( predicate ){ LOG(message, __VA_ARGS__); continue; }
#define RETURN_IF( predicate, message, ... ) if( predicate ){ DBG( message, __VA_ARGS__ ); return; }
#define DBGX(message,...) Logging::LogNoServer( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Debug, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACE(message,...) Logging::Log( Logging::MessageBase(message, ELogLevel::Trace, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
//#define TRACE_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(message, ELogLevel::Trace, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACEX(message,...) Logging::LogNoServer( Logging::MessageBase(message, ELogLevel::Trace, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )

#define LOGL(severity,message,...) Logging::Log( severity, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGX(severity,message,...) Logging::LogNoServer( Logging::MessageBase(message, severity, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGT(severity,message,...) Logging::Log( severity.Level, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOG(message,...) Logging::Log( _logLevel.Level, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGS(message,...) Logging::Log( Logging::Message( _logLevel.Level, message ) __VA_OPT__(,) __VA_ARGS__ )

#define LOG_IF(predicate, message,...) if( predicate ) Logging::Log( _logLevel.Level, Logging::MessageBase(message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOG_IFT(predicate, severity, message,...) if( predicate ) LOGT( severity, message __VA_OPT__(,) __VA_ARGS__ )
#define LOG_IFL(predicate, severity, message,...) if( predicate ) LOGL( severity, message __VA_OPT__(,) __VA_ARGS__ )
#define ERR_IF(predicate, message,...) LOG_IFL( predicate, ELogLevel::Error, message, __VA_ARGS__ )
#define LOG_MEMORY( severity, message, ... ) LogMemoryDetail( Logging::Message{severity, message} __VA_OPT__(,) __VA_ARGS__ );

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
	Γ α HaveLogger()noexcept->bool;
	Γ α ClearMemoryLog()noexcept->void;
	Γ α FindMemoryLog( uint32 messageId )noexcept->vector<Logging::Messages::ServerMessage>;
	struct LogTag{ sv Id; ELogLevel Level{ELogLevel::NoLog}; /*α Empty()const noexcept{return Id.empty() || Level==ELogLevel::None;}*/ };
	namespace Logging
	{
		Γ α DestroyLogger()->void;
		Γ α Initialize()noexcept->void;
		//Γ α Tags()noexcept->const array<Tag_,20>&;
		//Γ α ServerTags()noexcept->const array<Tag_,20>&;

		Γ α TagLevel( sv tag )noexcept->const LogTag&;
		//Γ α TagLevel( sv tag )noexcept->ELogLevel;
		//Γ α TagLevel( sv tagName, function<void(ELogLevel)> onChange, ELogLevel dflt=ELogLevel::NoLog )noexcept->ELogLevel;
		Γ α LogMemory()noexcept->bool;
		Γ α ServerLevel()noexcept->ELogLevel;
		Γ α Default()noexcept->spdlog::logger&;
	}

	constexpr PortType ServerSinkDefaultPort = 4321;

	namespace Logging
	{
		namespace Proto{class Status;}
		α StartTime()noexcept->TimePoint;
		Γ α SetStatus( const vector<string>& values )noexcept->void;
		α SetLogLevel( ELogLevel client, ELogLevel server )noexcept->void;
		α GetStatus()noexcept->up<Proto::Status>;
	}
#define var const auto
	Ξ FileName( const char* file_ )->string
	{
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
		}
		catch( const fmt::format_error& )
		{
			Log( Message(ELogLevel::Critical, "could not format {} - {}", source_location{m.File, m.LineNumber, m.Function}), m.MessageView, sizeof...(args) );
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
	template<class... Args>
	void Logging::LogOnce( const Logging::MessageBase& m, Args&&... args )noexcept
	{
		if( ShouldLogOnce(m) )
			Log( m, args... );
	}

	inline void Logging::LogNoServer( const Logging::MessageBase& m )noexcept
	{
		Default().log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
	}

	template<class... Args>
	inline void Logging::LogNoServer( const Logging::MessageBase& m, Args&&... args )noexcept
	{
		Default().log( SOURCE, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
	}
}

#pragma region MessageBase
namespace Jde::Logging
{
	template<class... Args>
	void LogMemoryDetail( Logging::Message&& m, Args&&... args )noexcept
	{
		vector<string> values; values.reserve( sizeof...(args) );
		ToVec::Append( values, args... );
		LogMemory( move(m), move(values) );
	}

	consteval MessageBase::MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, uint32 messageId, uint fileId, uint functionId )noexcept:
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
