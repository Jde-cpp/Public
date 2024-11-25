#pragma once
#include <jde/access/usings.h>
#include <jde/framework/coroutine/Await.h>

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Access{
	using PermissionRightPK=uint32;
	using PermissionRole=variant<PermissionRightPK,RolePK>;


	struct RoleRights final{
		Access::RolePK RolePK;
		Access::PermissionPK PermissionPK;
	};

	struct Role{
		RolePK PK;
		flat_set<PermissionPK> Permissions;
	};

	struct RoleLoadAwait final : TAwait<flat_map<RolePK,flat_set<PermissionRole>>>{
		RoleLoadAwait( sp<DB::AppSchema> schema )ι: _schema{schema}{};
	private:
		α Suspend()ι->void override;
		sp<DB::AppSchema> _schema;
	};
}