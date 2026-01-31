#pragma once
#include "usings.h"

namespace Jde::Access{
	struct IAdminAcl{
		β TestAdmin( str resource, str criteria, UserPK userPK, SRCE )ε->void=0;
	};
	struct IAcl : IAdminAcl{
		β Test( str schemaName, str resourceName, ERights rights, UserPK executer, SRCE )ε->void=0;
		β Rights( str schemaName, str resourceName, UserPK executer )ι->ERights=0;
	};
}