#pragma once
#include "usings.h"
namespace Jde::Access{
	struct IAuthorize{
		//α TestRead( str tableName, UserPK userId )ε->void;
		β Test( ERights rights, str resource, UserPK userPK, AppPK appPK )ε->void=0;
	};
}