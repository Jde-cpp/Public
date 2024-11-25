#include "Role.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>

#define let const auto

namespace Jde::Access{
	Ω loadRoles( sp<DB::AppSchema> schema, RoleLoadAwait& await )ι->DB::RowAwait::Task{
		try{
			auto permissionsTable = schema->GetTablePtr( "permissions" );
			auto roleMembersTable = schema->GetTablePtr( "role_members" );
			DB::SelectClause select{ roleMembersTable->Columns };
			select.Columns.push_back( permissionsTable->GetColumnPtr("is_role") );
			DB::FromClause from;
			from+=DB::Join{ roleMembersTable->GetColumnPtr("member_id"), permissionsTable->GetColumnPtr("permission_id"), true };
			DB::Statement statement{ move(select), move(from), {} };
			let rows = co_await roleMembersTable->Schema->DS()->SelectCo( statement.Move() );
			flat_map<RolePK,flat_set<PermissionRole>> roles;
			for( let& row : rows ){
				auto p = roles.try_emplace( row->GetUInt32(0), flat_set<PermissionRole>{} ).first;
				if( row->GetBit(2) )
					p->second.emplace( PermissionRole{std::in_place_index<1>, row->GetUInt32(1)} );
				else
					p->second.emplace( PermissionRole{std::in_place_index<0>, row->GetUInt32(1)} );
			}
			await.Resume( move(roles) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	α RoleLoadAwait::Suspend()ι->void{
		loadRoles( _schema, *this );
	}
}