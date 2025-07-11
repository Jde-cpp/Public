#include <jde/access/types/Permission.h>

#include <jde/db/IDataSource.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::Access{
	Permission::Permission( PermissionPK pk, Access::ResourcePK resourcePK, ERights allowed, ERights denied )ι:
		PK{pk}, ResourcePK{resourcePK}, Allowed{allowed}, Denied{denied}
	{}

	α getRights( const jobject& o, sv key )ι->ERights{
		auto rights{ ERights::None };
		if( let array = Json::FindArray(o, key); array )
			rights = ToRights( *array );
		else
			rights = (ERights)Json::AsNumber<uint8>(o, key);
		return rights;
	}
	Permission::Permission( const jobject& o )ι:
		PK{ Json::AsNumber<PermissionPK>(o, "id") },
		ResourcePK{ Json::FindNumberPath<Access::ResourcePK>(o, "resource/id").value_or(0) },
		Allowed{ getRights(o, "allowed") },
		Denied{ getRights(o, "denied") }
	{}

	α Permission::Update( optional<ERights> allowed, optional<ERights> denied )ι->void{
		if( allowed )
			Allowed = *allowed;
		if( denied )
			Denied = *denied;
	}
#ifdef notused
	α PermissionLoadAwait::Load()ι->DB::RowAwait::Task{
		flat_map<PermissionPK,Permission> permissions;
		try{
			let permissions = schema->GetTablePtr( "permissions" );
			let ds = permissions->Schema->DS();

			DB::Statement statement{ {permissions->Columns}, {permissions}, {} };
			let rows = co_await ds->SelectCo( move(statement) );
			flat_map<PermissionPK,Permission> permissions;
			for( let& row : rows ){
				auto pk = row->GetUInt32(0);
				roles.emplace( pk, {pk, row->GetUInt16(1), (ERights)row->GetUint8(2), (ERights)row->GetUint8(3)} );
			}
			await.Resume( move(roles) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}
#endif
}
