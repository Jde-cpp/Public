#pragma once
#ifndef JDE_SETTINGS_H //gcc precompiled headers
#define JDE_SETTINGS_H

#include "exports.h"
#include <jde/framework/io/json.h>

#define Φ Γ auto
namespace Jde::Settings{
	Φ Value()ι->const jvalue&;
	α FileStem()ι->string; //used as base path for logs.
	Φ Set( sv path, jvalue v, SRCE )ε->jvalue*;
	Φ Load()ι->void;

	Ξ AsObject( sv path, SRCE )ι->const jobject&{ return Json::AsObject(Value(), path, sl); }

	Ξ FindArray( sv path )ι->const jarray*{ return Json::FindArray(Value(), path); }
	Ξ FindBool( sv path )ι->optional<bool>{ return Json::FindBool(Value(), path); }
	Φ FindDuration( sv path )ι->optional<Duration>;
	Ξ FindObject( sv path )ι->const jobject*{ return Json::FindObject(Value(), path); }
	Ξ FindSV( sv path )ι->optional<sv>{ return Json::FindSV(Value(), path); }
	Φ FindString( sv path )ι->optional<string>;
	Φ FindStringArray( sv path )ι->vector<string>;
	Ξ FindPath( sv path )ι->optional<fs::path>{ auto s = FindString(path); return s ? fs::path{*s} : optional<fs::path>{}; }
	Ŧ FindNumber( sv path )ι->optional<T>{ return Json::FindNumber<T>(Value(), path); }

	Ξ FindDefaultArray( sv path )ι->const jarray&{ return Json::FindDefaultArray(Value(),path); };
	Ξ FindDefaultObject( sv path )ι->const jobject&{ return Json::FindDefaultObject(Value(), path); };
	template<IsEnum T, class ToEnum> α FindEnum( sv path, ToEnum&& toEnum )ι->optional<T>{ return Json::FindEnum<T,ToEnum>(Value(), path, toEnum); }
}
#undef Φ
#endif