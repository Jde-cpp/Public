#include <jde/opc/uatypes/Logger.h>

#define let const auto
namespace Jde::Opc{
	constexpr array<sv,14> EOpcLogTagstrings{ "iot",
		"uaNet", "uaSecure", "uaSession", "uaServer", "uaClient", "uaUser", "uaSecurity", "uaEvent", "uaPubSub", "uaDiscovery",
		"monitoring", "browse", "processingLoop" };

	α Format( const char* format, va_list ap )ι->string{
		string str{ format };
		/*if( str.ends_with("%.0s") ){
			va_list args; va_copy( args, ap );
			str.resize( str.size()-4 );
			std::cout << str << std::endl;
			let len = vsnprintf( 0, 0, str.c_str(), args );
			string m; m.resize( len + 1 );
			vsnprintf( m.data(), len + 1, str.c_str(), args );
			va_end(args);
			m.resize( len );  // remove the NULL
			return m;
		}*/
		//va_list args; va_copy( args, ap );
		//if( str.contains("%.*s") || str=="TCP %u\t| Creating server socket for \"%s\" on port %u" ){
			//std::cout << str << std::endl;
			va_list args2; va_copy( args2, ap );
			string out; out.reserve( str.size() );
			uint prev = 0;
			for( auto n = str.find('%'); n!=string::npos; n = str.find('%', n) ){
				out += str.substr(prev,n);
				char ch = str[++n];
				if(ch=='l' && str[n+1]=='u'){
					unsigned long lu = va_arg(args2, unsigned long);
					out += std::to_string(lu);
					++n;
				}else if( ch=='u' ){
					unsigned u = va_arg(args2, unsigned);
					out += std::to_string(u);
				}
				else if( ch=='i' ){
					int i = va_arg(args2, int);
					out += std::to_string(i);
				}
				else if( ch=='s' ){
					const char* psz = va_arg(args2, char*);
					out += psz;
				}
				else if( ch=='.' && str[n+1]=='*' && str[n+2]=='s' ){
					unsigned len = va_arg(args2, unsigned);
					char* psz = va_arg(args2, char*);
					string var{ psz, len };
					if( len>var.size() ){
						str.insert( n, len - var.length(), ' ');
						out += var;
					}else
						out += var.substr(0,len);
					n+=2;
				}
				else if( ch=='.' && isdigit(str[n+1]) ){
					char* psz = va_arg(args2, char*);
					string var{ psz };
					out += var.substr( 0, str[n+1]-'0' );
					++n;
				}
				else
					BREAK;
				prev = n;
			}
			if( prev<str.size() )
				out += str.substr( prev );
			return out;
		//}
/*		let len = vsnprintf( 0, 0, format, args );
		string m; m.resize( len + 1 );
		vsnprintf( m.data(), len + 1, format, args );
		va_end(args);
		m.resize( len );  // remove the NULL
		return m;
*/
	}
	α Clear( UA_Logger* /*context*/ )ι->void{}

	α UA_Log_Stdout_log( void *context, UA_LogLevel uaLevel, UA_LogCategory category, const char* file, const char* function, uint32_t line, const char *m, va_list args )ι->void{
		let level = (ELogLevel)( (int)uaLevel/100-1 ); //level==UA_LOGLEVEL_DEBUG=200
		uint tag_ = 1ull << (uint)(33ull+category);
		const EOpcLogTags tag = (EOpcLogTags)( tag_ );

		Log( level, (ELogTags)tag, spdlog::source_loc{file, (int)line, function}, "[{:x}]{}", (uint)context, Opc::Format(m,args) );
	}

	Logger::Logger( Handle uaHandle )ι:
		UA_Logger{ UA_Log_Stdout_log, (void*)uaHandle, Clear }
	{}
}
namespace Jde{
	α Opc::LogTagParser( sv name )ι->optional<ELogTags>{
		optional<ELogTags> tag;
		for( uint i=0; !tag && i<EOpcLogTagstrings.size(); ++i )
			if( EOpcLogTagstrings[i]==name )
				tag = (ELogTags)(1ull << (32+i));
		return tag;
	}
}