#pragma once
#include "usings.h"
namespace Jde::Access{
	struct IAcl{
		//α TestRead( str tableName, UserPK userId )ε->void;
		β Test( ERights rights, str resource, UserPK userPK )ε->void=0;

		string SchemaName;
	};
}