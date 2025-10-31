#include <jde/opc/uatypes/Logger.h>
#include <stacktrace>
#include <open62541/types.h>

#define let const auto
int mp_vsnprintf(char* s, size_t count, const char* format, va_list arg); //open62541/deps/mp_printf.h
namespace Jde::Opc{
	constexpr array<sv,14> EOpcLogTagstrings{ "opc",
		"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery",
		"monitoring", "browse", "processingLoop" };

	α Format( const char* format, va_list args )->string{
		UA_Byte buffer[512];
		UA_String str{ 512, buffer };
		let result = UA_String_vformat( &str, format, args );
		if( result!=UA_STATUSCODE_GOOD ){
			return Ƒ( "({:x})UA_String_vformat failed for: '{}'.", result, format );
		}
		const string y{ (char*)str.data, str.length };
		return y;
	}
	α Clear( UA_Logger* /*context*/ )ι->void{}

	α UA_Log_Stdout_log( void *context, UA_LogLevel uaLevel, UA_LogCategory category, const char *m, va_list args )ι->void{
		let level = (ELogLevel)( (int)uaLevel/100-1 ); //level==UA_LOGLEVEL_DEBUG=200
		uint tag_ = 1ull << (uint)(33ull+category);
		const EOpcLogTags tag = (EOpcLogTags)( tag_ );
		if( Logging::ShouldLog(level, (ELogTags)tag) ){
			let message = Opc::Format(m,args);
			Logging::LogStack( level, (ELogTags)tag, 2, "[{:x}]{}", (uint)context, message );
		}
	}
	/*
	α UA_Log_Stdout_log_file( void *context, UA_LogLevel uaLevel, UA_LogCategory category, const char* file, const char* function, uint32_t line, const char *m, va_list args )ι->void{
		let level = (ELogLevel)( (int)uaLevel/100-1 ); //level==UA_LOGLEVEL_DEBUG=200
		uint tag_ = 1ull << (uint)(33ull+category);
		const EOpcLogTags tag = (EOpcLogTags)( tag_ );

		Logging::Log( level, (ELogTags)tag, spdlog::source_loc{file, (int)line, function}, "[{:x}]{}", (uint)context, Opc::Format(m,args) );
	}
*/
	Logger::Logger( Handle uaHandle )ι:
		UA_Logger{ UA_Log_Stdout_log, (void*)uaHandle, Clear }
	{}
	α UALogParser::ToString( ELogTags tags )Ι->string{
		string y;
		for( uint i=0; i<EOpcLogTagstrings.size(); ++i ){
			if( (uint)tags & (1ull << (32+i)) )
				y += string{ EOpcLogTagstrings[i] }+'.';
		}
		if( y.size() )
			y.pop_back();
//		else
//			y = "NotFound";
		return y;
	}
	α UALogParser::ToTag( str name )Ι->ELogTags{
		ELogTags tag{};
		for( uint i=0; empty(tag) && i<EOpcLogTagstrings.size(); ++i )
			if( EOpcLogTagstrings[i]==name )
				tag = (ELogTags)(1ull << (32+i));
		return tag;
	}
}