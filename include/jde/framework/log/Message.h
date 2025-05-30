#pragma once
#include <stacktrace>
#ifndef MESSAGE_H
#define MESSAGE_H
#ifndef _MSC_VER
	#include <signal.h>
#endif

#ifdef NDEBUG
	#define BREAK
	#define BREAK_IF(x)
#else
	#ifdef _MSC_VER
		#define BREAK DebugBreak();
	#else
		#define BREAK if( CanBreak() ){ ::raise( 5/*SIGTRAP*/ ); }
	#endif
	#define BREAK_IF(x) if( x ){ BREAK; }
#endif
#define Φ Γ auto
#define let const auto
namespace Jde{
	Φ CanBreak()ι->bool;
}
namespace Jde::Logging{
	Φ BreakLevel()ι->ELogLevel;
	Γ auto Default()ι->spdlog::logger*;
	enum class EFields : uint16{ None=0, Timestamp=0x1, MessageId=0x2, Message=0x4, Level=0x8, FileId=0x10, File=0x20, FunctionId=0x40, Function=0x80, LineNumber=0x100, UserPK=0x200, User=0x400, ThreadId=0x800, Thread=0x1000, VariableCount=0x2000, SessionId=0x4000 };

#define FormatString const fmt::format_string<Args const&...>
#define ARGS const Args&
	template<ELogLevel TLevel, typename... Args>
	struct Logger{
		using enum ELogLevel;
		explicit Logger( ELogTags tags, FormatString&& m, ARGS... args, const spdlog::source_loc& sl )ι;
		explicit Logger( ELogTags tags, FormatString&& m, ARGS... args, SL sl )ι: Logger( tags, FWD(m), FWD(args)..., {sl.file_name(), (int)sl.line(), sl.function_name()} ){}
	};

	template<ELogLevel TLevel, typename... Args>
	α LogStack( ELogTags tags, std::stacktrace::size_type stackTraceIndex, FormatString&& m, ARGS... args )ι->void{
		if( auto fileMinLevel = FileMinLevel(tags); fileMinLevel==ELogLevel::NoLog || fileMinLevel>TLevel )
			return;

		let stacktrace = std::stacktrace::current();
		if( stacktrace.size() ){
			let& entry = stacktrace[ std::min(stacktrace.size()-1,stackTraceIndex+1) ];
			Logger<TLevel,Args...>{ tags, FWD(m), FWD(args)..., spdlog::source_loc{entry.source_file().c_str(), (int)entry.source_line(), nullptr} };
		}
		else{
			Logger<TLevel,Args...>{ tags, FWD(m), FWD(args)..., source_location::current() };
		}
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
template<typename... Args>
α Log( ELogLevel level, ELogTags tags, const spdlog::source_loc& sl, FormatString&& m, ARGS... args )ι->void{
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
#define CMD(Level) Logging::LogStack<ELogLevel::Level,Args...>( tags, stackTraceIndex+1, FWD(m), FWD(args)... )
template<typename... Args>
α Log( ELogLevel level, ELogTags tags, std::stacktrace::size_type stackTraceIndex, FormatString&& m, ARGS... args )ι->void{
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
template<typename... Args>
α Log( ELogLevel level, ELogTags tags, const std::source_location& sl, FormatString&& m, ARGS... args )ι->void{
	Log( level, tags, { sl.file_name(), (int)sl.line(), sl.function_name() }, FWD( m ), FWD( args )... );
}

namespace Logging{
	Φ Initialized()ι->bool;

	struct MessageBase{
		using ID=LogEntryPK;
		using ThreadID=uint;
		consteval MessageBase( sv message, ELogLevel level, const char* file, const char* function, uint_least32_t line, ID messageId=0, ID fileId=0, ID functionId=0 )ι;
		consteval MessageBase( sv message, const char* file, const char* function, uint_least32_t line )ι;
		consteval MessageBase( const char* file, const char* function, uint_least32_t line )ι;

		EFields Fields{ EFields::None };
		ELogLevel Level;
		mutable ID MessageId{0};
		sv MessageView;
		mutable ID FileId{0};
		const char* File;
		mutable ID FunctionId{0};
		const char* Function;
		uint_least32_t LineNumber;
		Jde::UserPK UserPK{0};
		ThreadID ThreadId{0};
		Γ MessageBase( ELogLevel level, sv message, const char* file, const char* function, uint_least32_t line )ι;
		Γ MessageBase( ELogLevel level, ID messageId, ID fileId, ID functionId, uint_least32_t line, Jde::UserPK userPK, ThreadID threadId )ι;
	protected:
		explicit Γ MessageBase( ELogLevel level, SL sl )ι;
	};

	struct Γ Message /*final*/ : MessageBase{
		Message( const MessageBase& b )ι;
		Message( const Message& x )ι;
		Message( ELogLevel level, string message, SRCE )ι;
		Message( ELogTags tags, ELogLevel level, string message, SRCE )ι;
		Message( ELogTags tags, ELogLevel level, string message, char const* file_, char const * function_, boost::uint_least32_t line_ )ι;

		ELogTags Tags;
		up<string> _pMessage;//TODO move to protected
		string _fileName;
	protected:
	};

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

	Φ LogMemory()ι->bool;
	Φ LogMemory( const Logging::MessageBase& messageBase, vector<string> values )ι->void;
	namespace External{
		Φ Log( const Logging::MessageBase& messageBase, const vector<string>& values )ι->void;
	}


	template<ELogLevel TLevel, typename... Args>
	Logger<TLevel,Args...>::Logger( ELogTags tags, FormatString&& m, ARGS... args, const spdlog::source_loc& sl )ι{
		if( Process::Finalizing() )
			return;
		if( !Logging::Initialized() ){
			sv msg{ m.get() };
			auto base = MessageBase{ TLevel, msg, sl.filename, sl.funcname, (uint_least32_t)sl.line };
			LogMemoryDetail( base, FWD(args)... );
			return;
		}
		if( auto fileMinLevel = FileMinLevel(tags); fileMinLevel!=NoLog && fileMinLevel<=TLevel ){
			try{
				if( auto p = Logging::Default(); p ){
					BREAK_IF( m.get().size()==0 );
					const auto level = (spdlog::level::level_enum)TLevel;
					p->log( sl, level, FWD(m), FWD(args)... );
				}
				else{
					auto& out = TLevel>Information ? std::cerr : std::cout;
					fmt::vformat_to( std::ostream_iterator<char>(out), m.get(), fmt::make_format_args(args...) );
				}
				BREAK_IF( tags<=ELogTags::Write && TLevel>=BreakLevel() );//don't want to break for opc server.
			}
			catch( const fmt::format_error& e ){
				Jde::Critical{ ELogTags::App, "could not format '{}' cargs: {} error: '{}'", m.get(), sizeof...(args), e.what() };
			}
			let logServer = !(tags & ELogTags::ExternalLogger);
			if( logServer || LogMemory() ){
			 	vector<string> values; values.reserve( sizeof...(args) );
			 	ToVec::Append( values, args... );
				MessageBase msg{ TLevel, sv{m.get().data(), m.get().size()}, sl.filename, sl.funcname, (uint_least32_t)sl.line };
			 	if( LogMemory() )
					LogMemory( msg, values );
			 	if( logServer )
			 		External::Log( msg, values );
			}
		}
	}
}}
#undef ARGS
#undef FormatString
#undef Φ
#undef let
#endif