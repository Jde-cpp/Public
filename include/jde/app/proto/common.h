#pragma once
#include "Common.pb.h"

namespace Jde::App::ProtoUtils{
	α ToException( IException&& e )ι->Jde::Proto::Exception;
	Ξ ToException( Jde::Proto::Exception&& e )ι->Jde::Exception{ return Jde::Exception{e.what(), e.code()}; }
	α ToQuery( string&& text, jobject&& variables, bool returnRaw )ι->Jde::Proto::Query;
}
