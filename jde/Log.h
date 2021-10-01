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

#define 🚪 JDE_NATIVE_VISIBILITY auto
namespace Jde::IO{ class IncomingMessage; }
namespace Jde
{
/*	template<class T>
	struct copy_ptr : unique_ptr<T>
	{
		using base=unique_ptr<T>;
		copy_ptr()noexcept=default;
		copy_ptr( const copy_ptr& rhs )noexcept:
			unique_ptr<T>{ rhs.get()==nullptr ? unique_ptr<T>{} : make_unique<T>(*rhs.get()) }
		{}
		copy_ptr( base&& rhs )noexcept:
			unique_ptr<T>( move(rhs) )
		{}
		copy_ptr& operator=( const base& rhs )
		{
			if( rhs.get()==nullptr )
				*this = nullptr;
			else
			{
				auto p = rhs.get();
				unique_ptr<T>& self = *this;
				self = make_unique<T>( *p );
			}
			return *this;
		}
	};*/
}
namespace Jde::Logging
{
#pragma region MessageBase
	namespace Messages{ struct Message; }
	//extern map<uint,set<uint>> OnceMessages; extern std::shared_mutex OnceMessageMutex;
	struct IServerSink;
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
		constexpr MessageBase( ELogLevel level, sv message, sv file, sv function, int line, uint32 messageId, uint fileId, uint functionId )noexcept;
		MessageBase( sv message, ELogLevel level, sv file, sv function, int line/*, uint messageId=0, uint fileId=0, uint functionId=0*/ )noexcept;
		virtual sv GetType()const{ return "MessageBase"; }
		EFields Fields{EFields::None};
		ELogLevel Level;
		uint MessageId{0};
		sv MessageView;
		uint FileId{0};
		sv File;
		uint FunctionId{0};
		sv Function;
		int LineNumber;
		uint UserId{0};
		uint ThreadId{0};
	};
	struct Message2 : MessageBase
	{
		Message2( const MessageBase& b )noexcept;
		Message2( const Message2& x )noexcept:
			MessageBase{ x },
			_pMessage{ x._pMessage ? make_unique<string>(*x._pMessage) : nullptr },
			_fileName{ x._fileName }
		{
			File = _fileName;
			if( _pMessage )
				MessageView = *_pMessage;
		}
		JDE_NATIVE_VISIBILITY Message2( ELogLevel level, string message, sv file, sv function, int line )noexcept;
		sv GetType()const override{ return "Message2"; }
	protected:
		unique_ptr<string> _pMessage;
		string _fileName;
	};

#pragma endregion
	void Log( Logging::MessageBase& messageBase )noexcept;
	ψ Log( Logging::MessageBase&& messageBase, Args&&... args )noexcept->void;

	🚪 ShouldLogOnce( const Logging::MessageBase& messageBase )noexcept->bool;
	🚪 LogOnce( Logging::MessageBase&& messageBase )noexcept->void;
	ψ LogOnce( Logging::MessageBase&& messageBase, Args&&... args )noexcept->void;
	void LogNoServer( Logging::MessageBase&& messageBase )noexcept;
	ψ LogNoServer( Logging::MessageBase&& messageBase, Args&&... args )noexcept->void;
	🚪 LogServer( const Logging::MessageBase& messageBase )noexcept->void;
	🚪 LogServer( const Logging::MessageBase& messageBase, vector<string>& values )noexcept->void;
	🚪 LogServer( Logging::Messages::Message& message )noexcept->void;
	🚪 LogMemory( const Logging::MessageBase& messageBase )noexcept->void;
	🚪 LogMemory( Logging::MessageBase&& messageBase )noexcept->void;
	🚪 LogMemory( Logging::MessageBase&& messageBase, vector<string> values )noexcept->void;
	🚪 LogMemory( Logging::Message2&& messageBase, vector<string> values )noexcept->void;
	🚪 LogMemory( const Logging::MessageBase& messageBase, vector<string> values )noexcept->void;
	ψ Tag( sv tag, Logging::MessageBase&& m, Args&&... args )noexcept->void;
}

#define MY_FILE __FILE__

#define CRITICAL(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Critical, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR0_ONCE(message) Logging::LogOnce( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) )
#define ERR(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERRX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(ELogLevel::Error, message##sv, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define WARN(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Warning, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define WARN_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(ELogLevel::Warning, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define WARN_IF(predicate, message,...) if( predicate ) Logging::Log( Logging::MessageBase(ELogLevel::Warning, message##sv, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define INFO(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Information, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define INFO_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(ELogLevel::Information, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Debug, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG_IF(predicate,message,...) if( predicate ) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Debug, message##sv, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define BREAK_IF( predicate, severity, message, ...) if( predicate ){ LOG(severity, message##sv, __VA_ARGS__); break; }
#define CONTINUE_IF( predicate, message, ...) if( predicate ){ DBG(message##sv, __VA_ARGS__); continue; }
#define RETURN_IF( predicate, message, ... ) if( predicate ){ DBG( message##sv, __VA_ARGS__ ); return; }
#define DBGX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Debug, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define DBG_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(ELogLevel::Debug, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACE(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define TRACE_ONCE(message,...) Logging::LogOnce( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
#define TRACEX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Trace, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )

#define LOG(severity,message,...) Logging::Log( Logging::MessageBase(message, severity, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGX(severity,message,...) Logging::LogNoServer( Logging::MessageBase(message, severity, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define LOGN(severity,message,messageId) Logging::Log( Logging::MessageBase(message, severity, MY_FILE, __func__, __LINE__, messageId) )
#define LOGS(severity,message,...) Logging::Log( Logging::Message2( severity, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )

#define LOG_IF(predicate, severity, message,...) if( predicate ) Logging::Log( Logging::MessageBase(severity, message##sv, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR_IF(predicate, message,...) LOG_IF( predicate, ELogLevel::Error, message, __VA_ARGS__ )
#define LOG_MEMORY( severity, message, ... ) LogMemoryDetail( Logging::Message2{ severity, message, MY_FILE, __func__, __LINE__} __VA_OPT__(,) __VA_ARGS__ );
#define TAG( tag, message, ... ) Logging::Tag( tag, Logging::MessageBase( ELogLevel::Trace, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )

namespace spdlog
{
#ifdef _MSC_VER
	namespace level{ enum level_enum : int;}
#endif
}
namespace Jde
{
	//extern std::shared_ptr<spdlog::logger> spLogger;
	🚪 DestroyLogger()->void;

   using namespace std::literals;
	🚪 HaveLogger()noexcept->bool;
	🚪 ClearMemoryLog()noexcept->void;
	🚪 FindMemoryLog( uint32 messageId )noexcept->vector<Logging::Messages::Message>;
	struct Tag_{ array<char,20> Id{0}; ELogLevel Level{ELogLevel::NoLog}; bool Empty()const noexcept{return Id[0]==0 || Level==ELogLevel::NoLog;} };
	namespace Logging
	{
		🚪 Initialize()noexcept->void;
		🚪 Tags()noexcept->const array<Tag_,20>&;
		🚪 ServerTags()noexcept->const array<Tag_,20>&;
		🚪 TagLevel( sv tag )noexcept->ELogLevel;
		🚪 TagLevel( sv tagName, function<void(ELogLevel)> onChange, ELogLevel dflt=ELogLevel::NoLog )noexcept->ELogLevel;
	}
#if _MSC_VER
	🚪 GetDefaultLogger()noexcept->spdlog::logger&;
	#define _logger GetDefaultLogger()
	🚪 GetServerSink()noexcept->Logging::IServerSink*;
	#define _pServerSink GetServerSink()
	🚪 ShouldLogMemory()noexcept->bool;
	#define _logMemory ShouldLogMemory()
	🚪 ServerLogLevel()noexcept->ELogLevel;
	#define _serverLogLevel ServerLogLevel()
	#define _tags Tags();
	#define _serverTags ServerTags();
#else
	extern spdlog::logger _logger;
	extern up<Logging::IServerSink> _pServerSink;
	extern ELogLevel _serverLogLevel;
	namespace Logging
	{
		extern bool _logMemory;
		extern array<Tag_,20> _tags;
		extern array<Tag_,20> _serverTags;
	}
#endif
	constexpr PortType ServerSinkDefaultPort = 4321;
	void SetServerSink( up<Logging::IServerSink> p )noexcept;
	void SetServerLevel( ELogLevel serverLevel )noexcept;

	namespace Logging
	{
		namespace Proto{class Status;}
		TimePoint StartTime()noexcept;
		🚪 SetStatus( const vector<string>& values )noexcept->void;
		void SetLogLevel( ELogLevel client, ELogLevel server )noexcept;
		up<Proto::Status> GetStatus()noexcept;
	}
#define var const auto
	inline string FileName( sv file )
	{
		if( file.starts_with('~') )
			return string{ file };
		uint start = 0;
		for( uint i=0; i<3 && (start = file.find( '/', start ))!=string::npos; ++i, ++start );

		auto homeDir = start==string::npos ? string{file} : '~'+string{ file.substr(start-1) };
		for( uint i{string::npos}; (i=homeDir.find("/./", 0, 3))!=string::npos; ++i )//{string::npos}:  -W sometimes-uninitialized
			homeDir.replace( i, 3, 1, '/' );

		for( uint end; (end=homeDir.find("/..", 0))!=string::npos; )
		{
			var start = homeDir.substr(0,end).find_last_of( '/' ); if( start==string::npos ) break;
			homeDir = homeDir.substr( 0, start )+homeDir.substr( end+3 );
		}
		return homeDir;
	}
#define SOURCE spdlog::source_loc{FileName(m.File.data()).c_str(),m.LineNumber,m.Function.data()}
	inline void Logging::Log( Logging::MessageBase& m )noexcept
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
		if( _logMemory )
			LogMemory( m );
		if( _pServerSink && _serverLogLevel<=m.Level )
			LogServer( move(m) );
	}

	template<class... Args>
	inline void Logging::Log( Logging::MessageBase&& m, Args&&... args )noexcept
	{//TODO just use format vs vformat catch fmt::v8::format_error in vformat version
		try
		{
			_logger.log( SOURCE, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
		}
		catch( const fmt::format_error& e )
		{
			CRITICAL( "could not format {} - {}", m.MessageView, sizeof...(args) );
		}
		if( auto pServer=_pServerSink.get(); pServer || _logMemory )
		{
			vector<string> values; values.reserve( sizeof...(args) );
			ToVec::Append( values, args... );
			if( _logMemory )
				LogMemory( m, values );
			if( pServer && _serverLogLevel<=m.Level )
				LogServer( m, values );
		}
	}
	template<class... Args>
	void Logging::LogOnce( Logging::MessageBase&& m, Args&&... args )noexcept
	{
		if( ShouldLogOnce(m) )
			Log( move(m), args... );
	}

	inline void Logging::LogNoServer( Logging::MessageBase&& m )noexcept
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)m.Level, m.MessageView );
	}

	template<class... Args>
	inline void Logging::LogNoServer( Logging::MessageBase&& m, Args&&... args )noexcept
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)m.Level, fmt::vformat(m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
	}
	namespace Internal
	{
		inline α TagLevel( sv tag, const array<Tag_,20>& tags )noexcept->ELogLevel
		{
			auto f = [tag](var x)noexcept->bool{ return x.Id[0]==0 || tag==(const char*)&x.Id[0]; };
			ELogLevel l{ELogLevel::NoLog};
			if( var p = find_if( tags.begin(), tags.end(), f); p!=tags.end() && p->Id[0]!=0 )
				l = p->Level;
			return l;
		}
	}
	ψ Logging::Tag( sv tag, Logging::MessageBase&& m, Args&&... args )noexcept->void
	{
		if( var l = Internal::TagLevel(tag, _tags); l>ELogLevel::NoLog )
			_logger.log( SOURCE, (spdlog::level::level_enum)l, fmt::vformat(m.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
		auto pServer = _pServerSink.get();
		if( !pServer && !_logMemory )
			return;
		if( var l = Internal::TagLevel(tag, _serverTags); l>ELogLevel::NoLog )
		{
			vector<string> values; values.reserve( sizeof...(args) );
			ToVec::Append( values, args... );
			m.Level = l;
			if( _logMemory )
				LogMemory( m, values );
			if( pServer && _serverLogLevel<=m.Level )
				LogServer( m, values );
		}
	}
}

#pragma region MessageBase
namespace Jde::Logging
{
	template<class... Args>
	void LogMemoryDetail( Logging::Message2&& m, Args&&... args )noexcept
	{
		vector<string> values; values.reserve( sizeof...(args) );
		ToVec::Append( values, args... );
		LogMemory( move(m), move(values) );
	}

	constexpr MessageBase::MessageBase( ELogLevel level, sv message, sv file, sv function, int line, uint messageId=0, uint fileId=0, uint functionId=0 )noexcept:
		Level{level},
		MessageId{ messageId ? messageId : IO::Crc::Calc32(message) },//{},
		MessageView{message},
		FileId{ fileId ? fileId : IO::Crc::Calc32(file) },
		File{file},
		FunctionId{ functionId ? functionId : IO::Crc::Calc32(function) },
		Function{function},
		LineNumber{line}
	{
		if( level!=ELogLevel::Trace )
			Fields |= EFields::Level;
		if( message.size() )
			Fields |= EFields::Message | EFields::MessageId;
		if( File.size() )
			Fields |= EFields::File | EFields::FileId;
		if( Function.size() )
			Fields |= EFields::Function | EFields::FunctionId;
		if( LineNumber )
			Fields |= EFields::LineNumber;
	}
}
#pragma endregion

#undef _logMemory
#undef 🚪
#undef SOURCE
#undef var