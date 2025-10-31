#pragma once
#include <jde/access/usings.h>
#include <jde/fwk/utils/collections.h>
#include "Permission.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	struct AllowedDisallowed final{
		ERights Allowed;
		ERights Denied;
	};
	struct User final{
		User( UserPK pk, bool deleted )ι:PK{pk}, IsDeleted{deleted}{}
		α operator+=( const Permission& permission )ι->User&;
		α ResourceRights( ResourcePK resource )Ι->AllowedDisallowed{ return FindDefault( Rights, resource ); }
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
		α Clear()ι->void{ Rights.clear(); Permissions.clear(); }

		UserPK PK;
		bool IsDeleted;
		flat_map<ResourcePK,AllowedDisallowed> Rights; //string=resourceName
		flat_map<PermissionPK,Permission> Permissions;
	};
}
