#include "AclHook.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/names.h>
#include <jde/ql/ql.h>
#include <jde/access/types/Resource.h>
#include <jde/access/Authorize.h>
#include "../accessInternal.h"

#define let const auto
namespace Jde::Access{
	α GetTable( str name )ε->sp<DB::View>;

	struct AclQLAwait final : TAwait<jvalue>{
		AclQLAwait( const QL::MutationQL& m, UserPK executer, uint pk, SL sl )ι:
			TAwait<jvalue>{ sl },
			Mutation{ m },
			PK{ pk },
			Executer{ executer }
		{}
		α Suspend()ι->void override;
		α InsertAcl()ι->void;

		QL::MutationQL Mutation;
		uint PK;
		Jde::UserPK Executer;
	private:
		α Table()ε->sp<DB::View>{ return GetTable( "acl" ); }
		α InsertPermission( const jobject& permission )ι->DB::ScalerAwait<PermissionPK>::Task;
		α InsertRole()ι->DB::ExecuteAwait::Task;
	};
	α AclQLAwait::Suspend()ι->void{
		InsertAcl();
	}

	α AclQLAwait::InsertAcl()ι->void{
		let& input = Mutation.Args;
		if( auto p = input.find("permissionRight"); p!=input.end() ) //identity{id:x}, permission:{ allowed:x, denied:x, resource:{id:x} }
			InsertPermission( Json::AsObject(p->value()) );
		else if( auto r = input.find("role"); r!=input.end() ) //identity{id:x}, role:{ id:x }
			InsertRole();
	}
	α AclQLAwait::InsertRole()ι->DB::ExecuteAwait::Task{
		jobject y;
		try{
			DB::InsertClause insert{ Table()->InsertProcName()+"_role" };
			let identityPK = Json::AsNumber<IdentityPK::Type>( Mutation.Args, "identity/id" );
			insert.Add( identityPK );
			let rolePK = Json::AsNumber<ResourcePK>( Mutation.Args, "role/id" );
			insert.Add( rolePK );
			y["rowCount"] = co_await DS()->ExecuteProcCo( insert.Move() );
			y["complete"] = true;
			//Authorizer().AddAcl( identityPK, rolePK );
			Resume( jvalue{y} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::InsertPermission( const jobject& permission )ι->DB::ScalerAwait<PermissionPK>::Task{
		jobject y;
		try{
			let allowed = (ERights)Json::FindNumber<uint8>( permission, "allowed" ).value_or(0);
			let denied = (ERights)Json::FindNumber<uint8>( permission, "denied" ).value_or(0);
			let resourcePK = Json::AsNumber<ResourcePK>( permission, "resource/id" );
			Authorizer().TestAdmin( resourcePK, Executer, _sl );
			DB::InsertClause insert{ Table()->UpsertProcName()+"_permission" };
			let identityPK = Json::AsNumber<IdentityPK::Type>( Mutation.Args, "identity/id" );
			insert.Add( identityPK );

			insert.Add( underlying(allowed) );
			insert.Add( underlying(denied) );
			insert.Add( resourcePK );
			let permissionPK = co_await DS()->ExecuteScaler<PermissionPK>( insert.Move() );
			y["permissionRight"].emplace_object()["id"] = permissionPK;
			y["complete"]=true;
			//Authorizer().AddAcl( identityPK, permissionPK, allowed, denied, resourcePK );
			Resume( y );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	struct AclQLSelectAwait final : TAwait<jvalue>{
		AclQLSelectAwait( const QL::TableQL& ql, UserPK executer, SL sl )ι:
			TAwait<jvalue>{ sl },
			Query{ ql },
			Executer{ executer }
		{}
		α Suspend()ι->void;
	private:
		α GetStatement( const QL::TableQL& childTable, sp<DB::Column> joinColumn )ε->optional<DB::Statement>;
		α LoadRoles( const QL::TableQL& permissionRightsQL )ι->DB::RowAwait::Task;
		α LoadPermissionRights( const QL::TableQL& permissionRightsQL )ι->DB::RowAwait::Task;
		α LoadPermissions( const QL::TableQL& permissionsQL )ι->DB::RowAwait::Task;
		QL::TableQL Query;
		Jde::UserPK Executer;
	};
	α AclQLSelectAwait::Suspend()ι->void{
		if( auto rights = Query.FindTable("permissionRights"); rights )
			LoadPermissionRights( *rights );
		else if( auto roles = Query.FindTable("roles"); roles )
			LoadRoles( *roles );
		else if( auto roles = Query.FindTable("permissions"); roles )
			LoadPermissions( *roles );
		else
			ResumeExp( Exception{"query not implemented"} );
	}
	α AclQLSelectAwait::GetStatement( const QL::TableQL& childTable, sp<DB::Column> joinColumn )ε->optional<DB::Statement>{
		let table = GetTable( "acl" );
		auto statement = QL::SelectStatement( childTable );
		if( auto aclStatement = !statement ? optional<DB::Statement>{} : QL::SelectStatement(Query); aclStatement ){
			statement->Select += move(aclStatement->Select);
			statement->Where += aclStatement->Where;
			statement->From += { joinColumn, table->GetColumnPtr("permission_id"), true };
			if( auto identities = Query.FindTable("identities"); identities ){
				if( !(identities->Columns.size()==1 && identities->Columns.front().JsonName=="id") )
					statement->From += { table->GetColumnPtr("identity_id"), GetTable("identities")->GetColumnPtr("identity_id"), true };
			}
		}
		return statement;
	}
	Ω addIdentityColumn( jobject& jrow, jobject*& identity, str key, DB::Value&& value )ι->void{
		if( !identity )
			identity = &jrow["identity"].emplace_object();
		(*identity)[key=="identityId" ? "id" : key] = value.Move();
	}
	α AclQLSelectAwait::LoadRoles( const QL::TableQL& roleQL )ι->DB::RowAwait::Task{
		jarray y;
		try{
			if( auto statement = GetStatement(roleQL, GetTable("roles")->GetColumnPtr("role_id")); statement ){
				auto rows = co_await DS()->SelectCo( statement->Move() );
				let& columns = statement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					jobject* role{};
					jobject* identity{};
					for( uint i=0; i<row->Size() && i<columns.size(); ++i ){
						let& column = columns[i];
						auto& value = (*row)[i];
						let key = DB::Names::ToJson(column->Name);
						if( column->Table->Name=="roles" ){
							if( !role )
								role = &jrow["role"].emplace_object();
							(*role)[key=="roleId" ? "id" : key] = value.Move();
						}
						else
							addIdentityColumn( jrow, identity, key, move(value) );
					}
					y.emplace_back( move(jrow) );
				}
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLSelectAwait::LoadPermissions( const QL::TableQL& permissionsQL )ι->DB::RowAwait::Task{
		jarray y;
		try{
			if( auto statement = GetStatement(permissionsQL, GetTable("permissions")->GetColumnPtr("permission_id")); statement ){
				auto rows = co_await DS()->SelectCo( statement->Move() );
				let& columns = statement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					for( uint i=0; i<row->Size() && i<columns.size(); ++i )
						Query.SetResult( jrow, columns[i], move((*row)[i]) );
					y.emplace_back( move(jrow) );
				}
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLSelectAwait::LoadPermissionRights( const QL::TableQL& permissionRights )ι->DB::RowAwait::Task{
		jarray y;
		try{
			if( auto statement = GetStatement(permissionRights, GetTable("permission_rights")->GetColumnPtr("permission_id")); statement ){
				auto rows = co_await DS()->SelectCo( statement->Move() );
				let& columns = statement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					jobject* right{};
					jobject* resource{};
					jobject* identity{};
					for( uint i=0; i<row->Size() && i<columns.size(); ++i ){
						let& column = columns[i];
						auto& value = (*row)[i];
						let key = DB::Names::ToJson(column->Name);
						if( column->Table->Name=="permission_rights" || column->Table->Name=="resources" ){
							if( !right )
								right = &jrow["permissionRight"].emplace_object();
							if( column->Table->Name=="permission_rights" )
								(*right)[key=="permissionId" ? "id" : key] = value.Move();
							else{
								if( !resource )
									resource = &(*right)["resource"].emplace_object();
								(*resource)[key] = value.Move();
							}
						}
						else
							addIdentityColumn( jrow, identity, key, move(value) );
					}
					y.emplace_back( move(jrow) );
				}
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α AclHook::Select( const QL::TableQL& ql, UserPK executer, SL sl )ι->HookResult{
		return ql.DBName()=="acl"
			? mu<AclQLSelectAwait>( ql, executer, sl )
			: nullptr;
	}

	α AclHook::InsertBefore( const QL::MutationQL& m, UserPK executer, SL sl )ι->HookResult{
		return m.TableName()=="acl" && m.Type==QL::EMutationQL::Create
			? mu<AclQLAwait>( m, executer, 0, sl )
			: nullptr;
	}
}