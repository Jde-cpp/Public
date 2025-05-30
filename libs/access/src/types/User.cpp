#include <jde/access/types/User.h>

#define let const auto
namespace Jde::Access{
	α User::UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void{
		auto permission = Permissions.find(permissionPK);
		if( permission==Permissions.end() )
			return;
		let resourcePK = permission->second.ResourcePK;
		permission->second.Update( allowed, denied );
		AllowedDisallowed rights{ ERights::None, ERights::None };
		for( auto&& [_,p] : Permissions ){
			if( p.ResourcePK==resourcePK ){
				p.Update( allowed, denied );
				if( allowed )
					rights.Allowed |= *allowed;
				if( denied )
					rights.Denied |= *denied;
			}
		}
		Rights[resourcePK] = rights;
	}

	α User::operator+=( const Permission& permission )ι->User&{
		Permissions.insert_or_assign( permission.PK, permission );
		ASSERT( permission.ResourcePK );
		auto& rights = Rights[permission.ResourcePK];
		rights.Allowed |= permission.Allowed;
		rights.Denied |= permission.Denied;

		return *this;
	}
}