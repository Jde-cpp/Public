#include <jde/fwk/io/file.h>
#include <jde/fwk/str.h>
#include <fstream>

#define let const auto
namespace Jde{
	constexpr ELogTags _tags{ ELogTags::IO };
	Ω fileSize( const fs::path& path )ε->uint{
		try{
			return fs::file_size( fs::canonical(path) );
		}
		catch( fs::filesystem_error& e ){
			throw IOException( move(e) );
		}
	}

	α IO::Load( const fs::path& path, SL sl )ε->string{
		CHECK_PATH( path, sl );
		auto size = fileSize( path );
		TRACESL( "Opening {} - {} bytes ", path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX(f.fail(), IOException(path, "Could not open file") );
		string y;
		y.reserve( size );
		y.assign( (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() );  //vexing parse
		return y;
	}

	α IO::LoadBinary( const fs::path& path, SL sl )ε->vector<char>{//fs::filesystem_error
		CHECK_PATH( path, sl );
		auto size = fileSize( path );
		TRACESL( "Opening {} - {} bytes ", path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );

		return vector<char>{ (std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>() };  //vexing parse
	}
#ifdef _WIN32
	α IO::BashToWindows( const fs::path& path )ι->fs::path{
		auto str = path.string();
		if( str.length()>3 && str[0]=='/' && str[1]!='/' && str[2]=='/' ){ //if /c/
			str[0] = str[1];
			str[1] = ':';
		}
		return fs::path{ "\\\\?\\"s+Str::Replace(str, '/', '\\') };
	}
#endif
}