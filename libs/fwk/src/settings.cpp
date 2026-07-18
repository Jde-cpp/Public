#include <jde/fwk/settings.h>
#include <iostream> // !important
#include <regex> // !important
#include <jde/fwk/chrono.h>
#include <jde/fwk/process/process.h>
#include <jde/fwk/str.h>
#include <jde/fwk/io/file.h>
#include <jde/fwk/log/MemoryLog.h>

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

	consteval α buildTypeSubDir()->sv{
		if constexpr( _debug )
			return "Debug";
		else
			return "Release";
	}

	Ω expandEnvVariable( string setting )ι->string{
		static const std::regex regex{ "\\$\\((.+?)\\)" };
		constexpr uint maxPasses{ 32 };//expansion is transitive but must be bounded - a value containing its own $(NAME) rescans forever.
		for( uint i=0;; ++i ){
			auto begin = std::sregex_iterator( setting.begin(), setting.end(), regex );
			if( begin==std::sregex_iterator() )
				break;
			if( i==maxPasses ){
				WARN( "'{}' - env expansion exceeded {} passes, cyclical $()?", setting, maxPasses );
				break;
			}
			let match = begin->str();
			let group = match.substr( 2, match.size()-3 );
			auto env = Process::GetEnv( group ).value_or( "" );
			if( env.empty() && group=="JDE_BUILD_TYPE" )
				env = buildTypeSubDir();
			if( env.empty() )
				DBG( "Environment variable '{}' not found", group );
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

	α Settings::FindPathArray( sv path )ι->vector<fs::path>{
		vector<fs::path> y;
		if( let jarray = Json::FindArray(Value(), path); jarray ){
			for( let& s : *jarray ){
				if( s.is_string() )
					y.push_back( expandEnvVariable(string{s.get_string()}) );
			}
		}
		return y;
	}

	α Settings::FileStem()ι->string{
		let executable = Process::Executable().filename();
	#ifdef _MSC_VER
			auto stem = executable.stem().string();
			return stem.size()>4 ? stem.substr( 4 ) : stem;
	#else
			return executable.string().starts_with( "Jde." ) ? fs::path( executable.string().substr(4) ) : executable;
	#endif
		}

	optional<vector<fs::path>> _importPaths;
	const vector<fs::path> _noImportPaths;//bound instead of _importPaths.value_or({}), which deep-copies the paths every Load.
	Ω path()ι->fs::path{
		static fs::path _path;
		if( !_path.empty() )
			return _path;
		let fileName = fs::path{ Ƒ("{}.jsonnet", Settings::FileStem()) };

		vector<fs::path> paths{ fileName, fs::path{"../config"}/fileName, fs::path{"config"}/fileName };
		if( auto settingsFile = _importPaths ? nullopt : Process::FindArg("-settings"); settingsFile ){
			if( Process::FindArg("-tests") ){
				_importPaths = vector<fs::path>{};
				#ifdef _WIN32
					string sqlTypePath = "sqlServer";
					settingsFile = IO::BashToWindows( *settingsFile ).string().substr( 4 ); //remove //?/
				#else
					string sqlTypePath = "mysql";
				#endif
				_importPaths->push_back( fs::path{*settingsFile}.parent_path()/"args"/sqlTypePath );
			}
			else if( auto cli = Process::FindArg("-include"); cli ){
				_importPaths = vector<fs::path>{};
				for( auto& relPath : Str::Split(*cli, ';') )
					_importPaths->push_back( fs::path{*settingsFile}.parent_path()/relPath );
			}
			paths = { fs::path{*settingsFile} };
		}

		auto p = find_if( paths, [](let& path){return fs::exists(path);} );
		_path = p!=paths.end() ? *p : Process::AppDataFolder()/fileName;
		std::cout << "settings path=" << _path.string() << std::endl;
		return _path;
	}
	α Settings::Directory()ι->fs::path{
		return path().parent_path();
	}

	α SetEnv( jobject& j )->void{
		for( auto& [key,value] : j ){
			if( value.is_string() )
				value = expandEnvVariable( string{value.get_string()} );
			else if( value.is_object() )
				SetEnv( value.get_object() );
		}
	}

	α Settings::Load()ι->void{
		let settingsPath = path();
		try{
			if( !fs::exists(settingsPath) )
				throw std::runtime_error{ Ƒ("file does not exist: '{}'", settingsPath.string()) };
			flat_map<string,string> args ={
				{ "buildTarget", _debug ? "debug" : "release" }
			};
			if( Process::FindArg("-tests") )
				args["logsDir"] = ( fs::current_path()/"logs" ).string();

			let settings = Json::TryReadJsonNet( settingsPath, _importPaths ? *_importPaths : _noImportPaths, args );
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
		boost::json::set_pointer_options opts;
		opts.create_objects = true;//create missing intermediate objects - replaces a hand-rolled fallback that corrupted the root.
		auto y = _settings->set_at_pointer( path, move(v), ec, opts );
		THROW_IFSL( !y, "Could not set '{}': {}", path, ec.message() );
		return y;
	}
}