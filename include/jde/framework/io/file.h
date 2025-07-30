#pragma once
#ifndef FILE_H
#define FILE_H
#define Φ Γ auto
namespace Jde::IO{
	Φ Load( const fs::path& path, SRCE )ε->string;
	Φ LoadBinary( const fs::path& path, SRCE )ε->vector<char>;
	Φ Save( const fs::path& path, string value, std::ios_base::openmode openMode, SRCE )ε->void;
	Ξ Save( const fs::path& path, string value, SRCE )ε->void{ Save( move(path), value, std::ios_base::out, sl ); }
	Φ SaveBinary( const fs::path& path, const vector<char>& values, SRCE )ε->void;
}
#undef Φ

#endif
