#pragma once
#ifndef FILE_H
#define FILE_H
#include <fstream>
#include <span>

#define Φ Γ auto
namespace Jde::IO{
	Φ Load( const fs::path& path, SRCE )ε->string;
	Φ LoadBinary( const fs::path& path, SRCE )ε->vector<char>;
	Ŧ SaveBinary( const fs::path& path, std::span<T> values, SRCE )ε->void;

#ifdef _WIN32
	α BashToWindows( const fs::path& path )ι->fs::path;
#endif
}
#undef Φ
namespace Jde{
	Ŧ IO::SaveBinary( const fs::path& path, std::span<T> data, SL sl )ε->void{
		std::ofstream f( path, std::ios::binary );
		THROW_IFX( f.fail(), IOException(path, "Could not open file", sl) );
		f.write( (char*)data.data(), data.size() );
	}
}
#endif
