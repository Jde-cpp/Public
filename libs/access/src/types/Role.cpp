#include "Role.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/Schema.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::Access{
	α LoadRoles( sp<DB::Schema> schema, RoleLoadAwait& await )ι->DB::RowAwait::Task{
		flat_map<RolePK,Role> roles;
		try{
			sp<DB::Table> roleRightsTable = schema->GetTablePtr( "role_rights" );
			let ds = roleRightsTable->Schema->DS();

			auto statement = DB::SelectSKsSql( roleRightsTable );
			let rows = co_await ds->SelectCo( move(statement) );
			flat_multimap<RolePK,PermissionPK> roles;
			for( let& row : rows )
				roles.emplace( row->GetUInt16(0), row->GetUInt16(1) );
			await.Resume( move(roles) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α RoleLoadAwait::Suspend()ι->void{
		LoadRoles( _schema, *this );
	}
}