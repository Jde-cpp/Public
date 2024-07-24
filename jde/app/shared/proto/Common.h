#pragma once

namespace Jde::Proto{
	α ToException( Jde::Proto::Exception&& e )ι->Jde::Exception{ return Jde::Exception{e.what(), e.code()}; }
}