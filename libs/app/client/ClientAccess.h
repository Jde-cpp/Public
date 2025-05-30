#include <jde/access/IAcl.h>

namespace Jde::App::Client{
	struct ClientAcl : Access::IAcl{
		//α TestRead( str tableName, UserPK userId )ε->void;
		α Test( str schemaName, str resourceName, Access::ERights rights, UserPK executer, SRCE )ε->void override;
	};
}