#pragma once
#include "IAccessIdentity.h"

namespace Jde::Access{
	using GroupPK=IdentityPK;
	struct User;

	struct Group final : IAccessIdentity{
		Group( GroupPK id )ι:Id(id){}

		GroupPK Id;
//		concurrent_flat_set<User> Users;
	};
}