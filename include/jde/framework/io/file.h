#pragma once
#ifndef FILE_H
#define FILE_H
/*
#include <string>
#include <functional>
#include <fstream>
#include <limits>
#include <memory>
#include <set>
#include <vector>
#include <boost/container/flat_map.hpp>
#include "../exports.h"
#include "../../../../../Framework/source/threading/Worker.h"
#include "../../../../../Framework/source/io/FileCo.h"
*/
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
