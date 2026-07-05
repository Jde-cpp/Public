#pragma once
#include <jde/access/usings.h>
#include <jde/fwk/utils/collections.h>
#include "Permission.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	struct AllowedDisallowed final{
		ERights Allowed;
		ERights Denied;
		α Effective()Ι->ERights{ return Allowed & ~Denied; }
	};
	struct User final{
		User( UserPK pk, string name, bool deleted )ι:PK{pk}, IsDeleted{deleted}, Name{move(name)}{}
		α operator+=( const Permission& permission )ι->User&;
		α ResourceRights( ResourcePK resource )Ι->AllowedDisallowed{ return FindDefault( Rights, resource ); }
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
		α Clear()ι->void{ Rights.clear(); Permissions.clear(); }

		UserPK PK;
		bool IsDeleted;
		string Name;
		flat_map<ResourcePK,AllowedDisallowed> Rights;
		flat_map<PermissionPK,Permission> Permissions;
	};
}
