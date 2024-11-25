#include "Permission.h"

#include <jde/db/IDataSource.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::Access{
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
