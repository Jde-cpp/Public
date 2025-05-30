#pragma once
#include "usings.h"

namespace Jde::Access{
	struct IAcl{
		//α TestRead( str tableName, UserPK userId )ε->void;
		β Test( str schemaName, str resourceName, ERights rights, UserPK executer, SRCE )ε->void=0;
	};
}