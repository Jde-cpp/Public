#pragma once
#include <jde/framework/str.h>
#include "exports.h"

#define Φ ΓDB auto

namespace Jde::DB::Names{
	Φ IsPlural( sv name )ι->bool;
	Ξ Capitalize( str name )ι->string{ ASSERT(name.size()>1); return string{(char)std::toupper(name[0])} + name.substr(1); }
	Φ FromJson( sv jsonName )ι->string;
	Φ ToJson( str schemaName )ι->string;
	Φ ToSingular( sv plural )ι->string;
	Φ ToPlural( sv singular )ι->string;
}
#undef Φ