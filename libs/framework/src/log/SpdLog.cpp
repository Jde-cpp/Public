#include <jde/framework/log/SpdLog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#ifdef _MSC_VER
	#include <crtdbg.h>
	#include <spdlog/spdlog.h>
	#include <spdlog/sinks/msvc_sink.h>
#endif
#include <jde/framework/settings.h>
#include <jde/framework/log/log.h>

#define let const auto

namespace Jde::Logging{
	using spdlog::level::level_enum;

	Ω loadSinks()ι->vector<spdlog::sink_ptr>{
		vector<spdlog::sink_ptr> sinks;
		let& sinkSettings = Settings::FindDefaultObject( "/logging/spd/sinks" );
		for( let& [name,sink] : sinkSettings ){
			spdlog::sink_ptr pSink;
			string additional;
			auto pattern =  Json::FindSV( sink, "/pattern" );
			if( name=="console" && Process::IsConsole() ){
				if( !pattern ){
					if constexpr( _debug ){
#ifdef _MSC_VER
							pattern = "\u001b]8;;file://%g\u001b\\%3!#-%3!l%$-%H:%M:%S.%e %v\u001b]8;;\u001b";
#else
						pattern = "%^%3!l%$-%H:%M:%S.%e \033]8;;file://%g#%#\a%v\033]8;;\a";
						//pattern = "%^%3!l%$-%H:%M:%S.%e %v %g#%#";//%-64@  %v
#endif
					}
					else
						pattern = "%^%3!l%$-%H:%M:%S.%e %v";//%-64@  %v
				}
				pSink = ms<spdlog::sinks::stdout_color_sink_mt>();
			}
			else if( name=="file" ){
				std::cout << "file sink:" << serialize(sink) << std::endl;
				optional<fs::path> pPath;
				if( auto p = Json::FindString(sink, "/path"); p )
					pPath = fs::path{ *p };
#pragma warning(disable: 4127)
				if( !_msvc && pPath && pPath->string().starts_with("/Jde-cpp") )
					pPath = fs::path{ "~/."+pPath->string().substr(1) };
				let markdown = Json::FindBool(sink, "/md" ).value_or( false );
				let fileNameWithExt = Settings::FileStem()+( markdown ? ".md" : ".log" );
				let path = pPath && !pPath->empty() ? *pPath/fileNameWithExt : Process::ApplicationDataFolder()/"logs"/fileNameWithExt;
				let truncate = Json::FindBool( sink, "/truncate" ).value_or( true );
				additional = Ƒ( " truncate='{}' path='{}'", truncate, path.string() );
				try{
					pSink = ms<spdlog::sinks::basic_file_sink_mt>( path.string(), truncate );
				}
				catch( const spdlog::spdlog_ex& e ){
					ERRT( ELogTags::Settings, "Could not create log:  ({}) path='{}' - {}", string{name}, path.string(), string{e.what()} );
					std::cerr << Ƒ( "Could not create log:  ({}) path='{}' - {}", name, path.string(), path.string(), e.what() ) << std::endl;
					continue;
				}
				if( !pattern )
					pattern = markdown ? "%^%3!l%$-%H:%M:%S.%e [%v](%g#L%#)\\" : "%^%3!l%$-%H:%M:%S.%e %-64@ %v";
			}
			else
				continue;
			pSink->set_pattern( string{*pattern} );
			let level = Json::FindEnum<ELogLevel>( sink, "/level", ToLogLevel ).value_or( ELogLevel::Trace );
			pSink->set_level( (level_enum)level );
			//std::cout << Ƒ( "({})level='{}' pattern='{}'{}", name, ToString(level), pattern, additional ) << std::endl;
			INFOT( ELogTags::Settings, "({})level='{}' pattern='{}'{}", name, ToString(level), *pattern, additional );
			sinks.push_back( pSink );
		}
		return sinks;
	}
	Ω logger()ι->spdlog::logger{
		auto sinks = loadSinks();
		spdlog::logger logger{ "my_logger", sinks.begin(), sinks.end() };

		let flushOn = Settings::FindEnum<ELogLevel>( "/logging/spd/flushOn", ToLogLevel ).value_or( _debug ? ELogLevel::Debug : ELogLevel::Information );
		logger.flush_on( (level_enum)flushOn );

		let minSinkLevel = std::accumulate( sinks.begin(), sinks.end(), ELogLevel::Critical, [](ELogLevel min, auto& p){ return std::min((ELogLevel)p->level(), min);} );
		logger.set_level( (level_enum)minSinkLevel );

		return logger;
	}

	SpdLog::SpdLog()ι:
		ILogger{ Settings::FindDefaultObject("/logging/spd") },
		_logger{ move(logger()) }{
		INFOT( ELogTags::Settings, "{} minLevel='{}' flushOn='{}' {}", Name(), Jde::ToString((ELogLevel)_logger.level()), Jde::ToString((ELogLevel)_logger.flush_level()), ToString() );
	}
}