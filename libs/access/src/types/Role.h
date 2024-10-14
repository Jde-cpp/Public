#pragma once

namespace Jde::DB{ struct Schema; }
namespace Jde::Access{
	using RolePK=uint16;
	using PermissionPK=uint16;

	struct RoleRights final{
		Access::RolePK RolePK;
		Access::PermissionPK PermissionPK;
	};

	struct Role{
		RolePK PK;
		flat_set<PermissionPK> Permissions;
	};

	struct RoleLoadAwait final : TAwait<flat_multimap<RolePK,PermissionPK>>{
		RoleLoadAwait( sp<DB::Schema> schema )ι: _schema{schema}{};
	private:
		α Suspend()ι->void override;
		sp<DB::Schema> _schema;
	};
}