#include "RoleLoadAwait.h"
#include <jde/ql/IQL.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };

	α RoleLoadAwait::Load()ι->QL::QLAwait<jarray>::Task{
		try{
			flat_map<RolePK,Role> y;
			let permissionQL = "roles{ id deleted permissionRights{id} }";
			let permissions = co_await *_qlServer->QueryArray( permissionQL, _executer );
			for( let& value : permissions ){
				const Role role{ Json::AsObject(value) };
				y.emplace( role.PK, role );
			}

			let roleQL = "roles{ id deleted roles{id} }";
			let roles = co_await *_qlServer->QueryArray( roleQL, _executer );
			for( let& value : roles ){
				const Role role{ Json::AsObject(value) };
				if( auto p = y.find(role.PK); p!=y.end() )
					p->second.Members.insert( role.Members.begin(), role.Members.end() );
				else
					y.emplace( role.PK, role );
			}
			for( let& [pk, role] : y )
				Trace{ _tags | ELogTags::Pedantic, "[{}]AddedRole membersSize={}", role.PK, role.Members.size() };
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

/*	α RoleLoadAwait::Load()ι->DB::RowAwait::Task{
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
*/
}