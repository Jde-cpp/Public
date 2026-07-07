#include <jde/access/AccessException.h>

namespace Jde::Access{
	α AccessException::Log()Ι->void{
		if( Level()==ELogLevel::NoLog || Process::Finalizing() )
			return;
		if( auto sv = Format(); sv.size() )
			Logging::Log( Logging::Entry{_sl, Level(), Tags, Executer, string{sv}, _args} );
		else
			Logging::Log( Logging::Entry{_sl, Level(), Tags, Executer, string{what()}} );
	}
}