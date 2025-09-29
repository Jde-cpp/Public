#pragma once
#ifndef FILE_H
#define FILE_H
#include <fstream>

#define Φ Γ auto
namespace Jde::IO{
	Φ Load( const fs::path& path, SRCE )ε->string;
	Φ LoadBinary( const fs::path& path, SRCE )ε->vector<char>;
	Φ Save( const fs::path& path, string value, std::ios_base::openmode openMode, SRCE )ε->void;
	Ξ Save( const fs::path& path, string value, SRCE )ε->void{ Save( move(path), value, std::ios_base::out, sl ); }
	Ŧ SaveBinary( const fs::path& path, std::span<T> values, SRCE )ε->void;
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
