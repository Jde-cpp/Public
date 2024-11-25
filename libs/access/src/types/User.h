#pragma once
#include "IAccessIdentity.h"
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>
#include <jde/framework/coroutine/TaskOld.h>
#include <jde/ql/QLHook.h>
#include <jde/db/awaits/MapAwait.h>
#include "../../../../../Framework/source/collections/Collections.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	using ResourcePK=uint16;
	struct Permission;
	struct AllowedDisallowed final{
//		α Update( optional<ERights> allowed, optional<ERights> denied )ι->void;
		ERights Allowed;
		ERights Denied;
	};
	struct User final : IAccessIdentity{
		User( uint pk, bool deleted )ι:PK{pk}, Deleted{deleted}{}
		α operator+=( const Permission& permission )ι->User&;
		α ResourceRights( ResourcePK resource )Ι->AllowedDisallowed{ return FindDefault( Rights, resource ); }
		α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
		α Clear()ι->void{ Rights.clear(); Permissions.clear(); }

		uint PK;
		bool Deleted;
		flat_map<ResourcePK,AllowedDisallowed> Rights; //string=resourceName
		flat_map<PermissionPK,Permission> Permissions;
	};

	struct Identities{
		flat_map<UserPK,User> Users;
		flat_multimap<GroupPK,UserPK> GroupMembers;
	};

	struct UserLoadAwait final : TAwait<Identities>{
		UserLoadAwait( sp<DB::AppSchema> schema )ι;
		α Suspend()ι->void override{ Execute(); }
	private:
		α Execute()ι->DB::MapAwait<IdentityPK,optional<TimePoint>>::Task;
		sp<DB::AppSchema> _schema;
	};

	struct UserGraphQL final : QL::IQLHook{
		α Select( const QL::TableQL& ql, UserPK userPK, SRCE )ι->up<TAwait<jvalue>> override;
	};
}