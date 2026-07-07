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
			throw IO::IOException( move(e) );
		}
	}

	α IO::Load( const fs::path& path, SL sl )ε->string{
		CHECK_PATH( path, sl );
		let size = fileSize( path );
		TRACESL( "Opening {} - {} bytes ", path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IO::IOException(path, "Could not open file", sl) );
		string y( size, '\0' );
		f.read( y.data(), (std::streamsize)size );
		THROW_IFX( (uint)f.gcount()!=size, IO::IOException(path, Ƒ("Read {} of {} bytes.", f.gcount(), size), sl) );
		return y;
	}

	α IO::LoadBinary( const fs::path& path, SL sl )ε->vector<char>{//fs::filesystem_error
		CHECK_PATH( path, sl );
		let size = fileSize( path );
		TRACESL( "Opening {} - {} bytes ", path.string(), size );
		std::ifstream f( path, std::ios::binary ); THROW_IFX( f.fail(), IO::IOException(path, "Could not open file", sl) );
		vector<char> y( size );
		f.read( y.data(), (std::streamsize)size );
		THROW_IFX( (uint)f.gcount()!=size, IO::IOException(path, Ƒ("Read {} of {} bytes.", f.gcount(), size), sl) );
		return y;
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