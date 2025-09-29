#include <jde/framework/settings.h>
#include <regex>
#include <jde/framework/chrono.h>
#include <jde/framework/process/process.h>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include <jde/framework/log/MemoryLog.h>
#include <jde/framework/log/Logger.h>

#define let const auto

namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Settings };
	up<jvalue> _settings; //up so doesn't get initialized 2x.
	α Settings::Value()ι->const jvalue&{
		if( !_settings )
			Load();
		return *_settings;
	}

	α Settings::FindDuration( sv path, SL sl )ι->optional<Duration>{
		optional<Duration> y;
		if( let setting = FindSV(path); setting.has_value() )
			y = Chrono::TryToDuration( string{*setting}, ELogLevel::Error, sl );
		return y;
	}

	Ω expandEnvVariable( string setting )ι->string{
		std::regex regex( "\\$\\((.+?)\\)" );
		for(;;){
			auto begin = std::sregex_iterator( setting.begin(), setting.end(), regex );
			if( begin==std::sregex_iterator() )
				break;
			std::smatch b = *begin;
			let match = begin->str();
			let group = match.substr( 2, match.size()-3 );
			let env = Process::EnvironmentVariable( group ).value_or( "" );
			setting = Str::Replace( setting, match, env );
		}
		return setting;
	}
	α Settings::FindString( sv path )ι->optional<string>{
		auto setting = Json::FindString( Value(), path );
		if( setting )
			setting = expandEnvVariable( *setting );
		return setting;
	}

	α Settings::FindStringArray( sv path )ι->vector<string>{
		vector<string> y;
		if( let jarray = Json::FindArray(Value(), path); jarray ){
			for( let& s : *jarray ){
				if( s.is_string() )
					y.push_back( expandEnvVariable(string{s.get_string()}) );
			}
		}
		return y;
	}

	consteval α buildTypeSubDir()->sv{
		if constexpr( _debug )
			return "Debug";
		else
			return "Release";
	}

	α Settings::FileStem()ι->string{
		let executable = Process::Executable().filename();
	#ifdef _MSC_VER
			auto stem = executable.stem().string();
			return stem.size()>4 ? stem.substr(4) : stem;
	#else
			return executable.string().starts_with( "Jde." ) ? fs::path( executable.string().substr(4) ) : executable;
	#endif
		}

	α Path()ι->fs::path{
		static fs::path _path;
		let fileName = fs::path{ Ƒ("{}.jsonnet", Settings::FileStem()) };

		vector<fs::path> paths{ fileName, fs::path{"../config"}/fileName, fs::path{"config"}/fileName };
		if( let cli = Process::FindArg("-settings"); cli ){
			if( Process::FindArg( "-tests" ) ){
				#ifdef _MSC_VER
					string osPath = "win";
				#else
					string osPath = "linux";
				#endif
				Json::AddImportPath( fs::path{*cli}.parent_path()/osPath/buildTypeSubDir() );
			}
			paths = { fs::path{*cli} };
		}

		auto p = find_if( paths, []( let& path ){return fs::exists(path);} );
		_path = p!=paths.end() ? *p : Process::ApplicationDataFolder()/fileName;
		std::cout << "settings path=" << _path.string() << std::endl;
		return _path;
	}
	α SetEnv( jobject& j )->void{
		for( auto& [key,value] : j ){
			if( value.is_string() ){
				string setting{ value.get_string() };
				std::regex regex( "\\$\\((.+?)\\)" );
				for(;;){
					auto begin = std::sregex_iterator( setting.begin(), setting.end(), regex );
					if( begin==std::sregex_iterator() )
						break;
					std::smatch b = *begin;
					let match = begin->str();
					let group = match.substr( 2, match.size()-3 );
					auto env = Process::EnvironmentVariable( group ).value_or( "" );
					if( env.empty() && group=="JDE_BUILD_TYPE" )
						env = buildTypeSubDir();
					if( env.empty() )
						DBG( "Environment variable '{}' not found", group );

					setting = Str::Replace( setting, match, env );
				}
				value = setting;
			}
			else if( value.is_object() )
				SetEnv( value.get_object() );
		}
	}

	α Settings::Load()ι->void{
		let settingsPath = Path();
		try{
			if( !fs::exists(settingsPath) )
				throw std::runtime_error{ Ƒ("file does not exist: '{}'", settingsPath.string()) };
			let settings = Json::TryReadJsonNet( settingsPath );
			if( !settings )
				throw std::runtime_error{ settings.error() };
			_settings = mu<jvalue>( *settings );
			if( _settings->is_object() )
				SetEnv( _settings->get_object() );
			INFOT( ELogTags::App, "Settings path={}", settingsPath.string() );
		}
		catch( const std::exception& e ){
			_settings = mu<jvalue>( jobject{{"error", e.what()}} );
			std::cerr << e.what() << std::endl;
			CRITICAL( "({})Could not load settings - {}", settingsPath.string(), e.what() );
			BREAK;
		}
	}
	α Settings::Set( sv path, jvalue v, SL sl )ε->jvalue*{
		ASSERT( _settings );
		boost::system::error_code ec;
		let y = _settings->set_at_pointer( path, move(v), ec );
		if( ec ){
			let parts = Str::Split( path, '/' );
			auto& object = _settings->get_object();
			for( uint i=0; i<(parts.size() ? parts.size()-1 : 0); ++i ){
				if( auto p = object.find(parts[i]); p!=object.end() ){
					THROW_IFSL( !p->value().is_object(), "Could not set '{}' to '{}'", path, serialize(v) );
					object = p->value().get_object();
				}
				else
					object = object.emplace( parts[i], jobject{} ).first->value().get_object();
			}
			//let x = serialize(*_settings);
			object.emplace( parts.back(), move(v) );
		}
		return y;
	}
}