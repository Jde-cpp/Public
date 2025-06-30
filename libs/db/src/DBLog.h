#pragma once
#include <jde/db/exports.h>

#define Φ ΓDB auto
namespace Jde::DB{
	struct Sql; struct Value;
	α LogDisplay( const Sql& sql, string error={} )ι->string;
	Φ Log( const Sql& sql, SL sl )ι->void;
	α Log( const Sql& sql, ELogLevel level, string error, SL sl )ι->void;
	α LogNoServer( const Sql& sql, ELogLevel level, string error, SL sl )ι->void;
}
#undef Φ