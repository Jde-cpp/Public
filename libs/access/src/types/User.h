#pragma once
#include "IAccessIdentity.h"
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/ql/QLHook.h>
#include <jde/db/awaits/MapAwait.h>
#include "../../../../../Framework/source/collections/Collections.h"
#include "../accessInternal.h"
#include "Permission.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	struct AllowedDisallowed final{
		ERights Allowed;
		ERights Denied;
	};
	struct User final{
		User( UserPK pk, bool deleted )ι:PK{pk}, Deleted{deleted}{}
		α operator+=( const Permission& permission )ι->User&;
		α ResourceRights( ResourcePK resource )Ι->AllowedDisallowed{ return FindDefault( Rights, resource ); }
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
		α Clear()ι->void{ Rights.clear(); Permissions.clear(); }

		UserPK PK;
		bool Deleted;
		flat_map<ResourcePK,AllowedDisallowed> Rights; //string=resourceName
		flat_map<PermissionPK,Permission> Permissions;
	};

	struct UserGraphQL final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	};
}