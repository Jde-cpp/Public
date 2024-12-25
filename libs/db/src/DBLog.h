#pragma once
//#include "exports.h"

//#define Φ ΓDB auto
namespace Jde::DB{
	struct Value;
	α LogDisplay( sv sql, const vector<Value>* pParameters, string error={} )ι->string;
	α Log( sv sql, const vector<Value>* pParameters, SL sl )ι->void;
	α Log( sv sql, const vector<Value>* pParameters, ELogLevel level, string error, SL sl )ι->void;
	α LogNoServer( string sql, const vector<Value>* pParameters, ELogLevel level, string error, SL sl )ι->void;
}
//#undef Φ