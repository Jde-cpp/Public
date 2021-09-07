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
	extern map<uint,set<uint>> OnceMessages; extern std::shared_mutex OnceMessageMutex;
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
	void Log( Logging::MessageBase& messageBase );
	template<class... Args>
	void Log( Logging::MessageBase&& messageBase, Args&&... args )noexcept;

	🚪 ShouldLogOnce( const Logging::MessageBase& messageBase )->bool;
	🚪 LogOnce( Logging::MessageBase&& messageBase )->void;
	template<class... Args>
	void LogOnce( Logging::MessageBase&& messageBase, Args&&... args );
	void LogNoServer( Logging::MessageBase&& messageBase );
	template<class... Args>
	void LogNoServer( Logging::MessageBase&& messageBase, Args&&... args );
	🚪 LogServer( const Logging::MessageBase& messageBase )noexcept->void;
	🚪 LogServer( const Logging::MessageBase& messageBase, vector<string>& values )noexcept->void;
	🚪 LogServer( Logging::Messages::Message& message )noexcept->void;
	🚪 LogMemory( const Logging::MessageBase& messageBase )noexcept->void;
	🚪 LogMemory( Logging::MessageBase&& messageBase )noexcept->void;
	🚪 LogMemory( Logging::MessageBase&& messageBase, vector<string> values )noexcept->void;
	🚪 LogMemory( Logging::Message2&& messageBase, vector<string> values )noexcept->void;
	🚪 LogMemory( const Logging::MessageBase& messageBase, vector<string> values )noexcept->void;
}

#define MY_FILE __FILE__

#define CRITICAL(message,...) Jde::Logging::Log( Jde::Logging::MessageBase(Jde::ELogLevel::Critical, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERR0_ONCE(message) Logging::LogOnce( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) )
#define ERR(message,...) Logging::Log( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__) __VA_OPT__(,) __VA_ARGS__ )
#define ERRX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Error, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
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
#define RETURN_IF( predicate, message, ...) if( predicate ){ DBG(message##sv, __VA_ARGS__); return; }
#define DBGX(message,...) Logging::LogNoServer( Logging::MessageBase(ELogLevel::Debug, message, MY_FILE, __func__, __LINE__), __VA_ARGS__ )
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
	🚪 InitializeLogger()noexcept->void;
	🚪 HaveLogger()noexcept->bool;
	🚪 ClearMemoryLog()noexcept->void;
	🚪 FindMemoryLog( uint32 messageId )noexcept->vector<Logging::Messages::Message>;
#if _MSC_VER
	🚪 GetDefaultLogger()noexcept->spdlog::logger&;
	#define _logger GetDefaultLogger()
	🚪 GetServerSink()noexcept->Logging::IServerSink*;
	#define _pServerSink GetServerSink()
	🚪 ShouldLogMemory()noexcept->bool;
	#define _logMemory ShouldLogMemory()
	🚪 ServerLogLevel()noexcept->ELogLevel;
	#define _serverLogLevel ServerLogLevel()
#else
	extern spdlog::logger _logger;
	extern up<Logging::IServerSink> _pServerSink;
	extern ELogLevel _serverLogLevel;
	namespace Logging{ extern bool _logMemory; }
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

	inline std::string FileName( std::string_view file )
	{
		uint start = 0;
		for( uint i=0; i<3 && start!=std::string::npos; ++i, start+=1 )
			start = file.find( '/', start );
		return start==std::string::npos ? std::string{file} : '~'+std::string{ file.substr(start-1) };
	}
#define SOURCE spdlog::source_loc{FileName(messageBase.File.data()).c_str(),messageBase.LineNumber,messageBase.Function.data()}
	inline void Logging::Log( Logging::MessageBase& messageBase )
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)messageBase.Level, messageBase.MessageView );
		if( _logMemory )
			LogMemory( messageBase );
		if( _pServerSink && _serverLogLevel<=messageBase.Level )
			LogServer( move(messageBase) );
	}

	template<class... Args>
	inline void Logging::Log( Logging::MessageBase&& messageBase, Args&&... args )noexcept
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)messageBase.Level, fmt::vformat(messageBase.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
		if( auto pServer=_pServerSink.get(); pServer || _logMemory )
		{
			vector<string> values; values.reserve( sizeof...(args) );
			ToVec::Append( values, args... );
			if( _logMemory )
				LogMemory( messageBase, values );
			if( pServer && _serverLogLevel<=messageBase.Level )
				LogServer( messageBase, values );
		}
	}
	template<class... Args>
	void Logging::LogOnce( Logging::MessageBase&& messageBase, Args&&... args )
	{
		if( ShouldLogOnce(messageBase) )
			Log( move(messageBase), args... );
	}

	inline void Logging::LogNoServer( Logging::MessageBase&& messageBase )
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)messageBase.Level, messageBase.MessageView );
	}

	template<class... Args>
	inline void Logging::LogNoServer( Logging::MessageBase&& messageBase, Args&&... args )
	{
		_logger.log( SOURCE, (spdlog::level::level_enum)messageBase.Level, fmt::vformat(messageBase.MessageView, fmt::make_format_args(std::forward<Args>(args)...)) );
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