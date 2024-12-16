#include "AclHook.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/names.h>
#include <jde/ql/ql.h>
#include "../types/Resource.h"
#include "../Authorize.h"
#include "../accessInternal.h"

#define let const auto
namespace Jde::Access{
	α GetTable( str name )ε->sp<DB::View>;
	α AuthorizeAdmin( ResourcePK resourcePK, UserPK userPK, SL sl )ε->void;
	α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
	α CreateResource( Resource&& args )ε->void;
	α UpdateResourceDeleted( str schemaName, const jobject& args, bool restored )ε->void;
	α RemoveFromRole(	RolePK rolePK, flat_set<PermissionIdentityPK> toRemove )ι->void;

	struct AclQLAwait final : TAwait<jvalue>{
		AclQLAwait( const QL::MutationQL& m, UserPK userPK, uint pk, SL sl )ι:
			TAwait<jvalue>{ sl },
			Mutation{ m },
			PK{ pk },
			UserPK{ userPK }
		{}
		α Suspend()ι->void override;
		α InsertAcl()ι->void;
		QL::MutationQL Mutation;
		uint PK;
		Jde::UserPK UserPK;
	private:
		α Table()ε->sp<DB::View>{ return GetTable( "acl" ); }
		α InsertPermission( const jobject& permission )ι->DB::ScalerAwait<PermissionPK>::Task;
		α InsertRole()ι->DB::ExecuteAwait::Task;
	};

	α AclQLAwait::InsertAcl()ι->void{
		let& input = Mutation.Input();
		if( auto p = input.find("permission"); p!=input.end() ) //identityId:x, permission:{ allowed:x, denied:x, resource:{id:x} }
			InsertPermission( Json::AsObject(p->value()) );
		else if( auto r = input.find("role"); r!=input.end() ) //identityId:x, role:{ id:x }
			InsertRole();
	}
	α AclQLAwait::InsertRole()ι->DB::ExecuteAwait::Task{
		jobject y;
		try{
			DB::InsertClause insert{ Table()->InsertProcName()+"_role" };
			let identityPK = Json::AsNumber<IdentityPK::Type>( Mutation.Input(), "identityId" );
			insert.Add( identityPK );
			let rolePK = Json::AsNumber<ResourcePK>( Mutation.Input(), "role/id" );
			insert.Add( rolePK );
			y["rowCount"] = co_await DS()->ExecuteProcCo( insert.Move() );
			y["complete"] = true;
			Authorizer().AddAcl( identityPK, rolePK );
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
			AuthorizeAdmin( resourcePK, UserPK, _sl );
			DB::InsertClause insert{ Table()->UpsertProcName()+"_permission" };
			let identityPK = Json::AsNumber<IdentityPK::Type>( Mutation.Input(), "identityId" );
			insert.Add( identityPK );

			insert.Add( underlying(allowed) );
			insert.Add( underlying(denied) );
			insert.Add( resourcePK );
			let permissionPK = co_await DS()->ExecuteScaler<PermissionPK>( insert.Move() );
			y["permission"].emplace_object()["id"] = permissionPK;
			y["complete"]=true;
			Authorizer().AddAcl( identityPK, permissionPK, allowed, denied, resourcePK );
			Resume( y );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

/*	α AclQLAwait::AssignToRole( RolePK rolePK, flat_set<PermissionIdentityPK> members )ι->DB::MapAwait<PermissionIdentityPK,bool>::Task{
		try{
			auto table = GetTable( "permissions" );
			DB::WhereClause where{ table->GetColumnPtr("permission_id"), DB::ToValue(members) };
			DB::Statement s{ table->Columns, {table}, move(where) };
			let pkIsRole = co_await table->Schema->DS()->SelectMap<PermissionIdentityPK,bool>( s.Move() );
			flat_set<PermissionRole> members;
			for( let [pk,isRole] : pkIsRole ){
				if( isRole )
					members.emplace( PermissionRole{std::in_place_index<1>, pk} );
				else
					members.emplace( PermissionRole{std::in_place_index<0>, pk} );
			}
			Access::AssignToRole( rolePK, move(members) );
			Resume( jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
*/
	α AclQLAwait::Suspend()ι->void{
		try{
			using enum QL::EMutationQL;
			let table = GetTable( Mutation.TableName() );
			if( table->Name=="permissions" && Mutation.Type==Update ){//id:x, input:{ allowed:y, denied:z }}
				let allowed = Json::FindNumber<uint8>( Mutation.Input(), "allowed" );
				let denied = Json::FindNumber<uint8>( Mutation.Input(), "denied" );
				UpdatePermission( Json::AsNumber<PermissionPK>(Mutation.Args, "id"), allowed ? optional<ERights>((ERights)*allowed) : nullopt, denied ? optional<ERights>((ERights)*denied) : nullopt );
			}
			else if( table->Name=="acl" ){
				if( Mutation.Type==Create ){
					InsertAcl();
					return;
				}
			}
			else if( table->Name=="users" ){
				if( Mutation.Type==Create )
					Authorizer().CreateUser( Jde::UserPK{PK} );
				else if( Mutation.Type==Delete )
					Authorizer().DeleteUser( Jde::UserPK{Mutation.Id()} );
				else if( Mutation.Type==Restore )
					Authorizer().RestoreUser( Jde::UserPK{Mutation.Id()} );
			}
			else if( table->Name=="identityGroups" ){
				switch( Mutation.Type ){
				case Purge:
				case Delete:
					Authorizer().DeleteGroup( GroupPK{PK} );
					break;
				case Restore:
					Authorizer().RestoreGroup( GroupPK{PK} );
					break;
				case Add:
				case Remove:{
					auto members = Json::AsArray( Mutation.Args, "members" );
					flat_set<IdentityPK::Type> memberPKs; memberPKs.reserve( members.size() );
					for( let& member : members )
						memberPKs.emplace( Json::AsNumber<IdentityPK::Type>(member) );
					if( Mutation.Type==Add )
						Authorizer().AddToGroup( GroupPK{PK}, move(memberPKs) );
					else if( Mutation.Type==Remove )
						Authorizer().RemoveFromGroup( GroupPK{PK}, move(memberPKs) );
					break;}
					default: break;
				}
			}
			else if( table->Name=="roles" ){
				switch( Mutation.Type ){
				case Purge:
				case Delete:
					Authorizer().DeleteRestoreRole( PK, true );
					break;
				case Restore:
					Authorizer().DeleteRestoreRole( PK, false );
					return;
/*				case Add:
				case Remove:{
					auto members = Json::AsArray( Mutation.Args, "members" );
					flat_set<IdentityPK> memberPKs; memberPKs.reserve( members.size() );
					for( let& member : members )
						memberPKs.emplace( Json::AsNumber<IdentityPK>(member) );
					if( Mutation.Type==Add )
						AssignToRole( PK, move(memberPKs) );
					else if( Mutation.Type==Remove )
						RemoveFromRole( PK, move(memberPKs) );
					break;}*/
				default: break;
				}
			}
			else if( table->Name=="resources" ){
				if( Mutation.Type==Create )
					CreateResource( {PK, Mutation.Input()} );
				else if( Mutation.Type==Delete || Mutation.Type==Restore )
					UpdateResourceDeleted( table->Schema->Name, Mutation.Args, Mutation.Type==Restore );
			}
			Resume( jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	struct AclQLSelectAwait final : TAwait<jvalue>{
		AclQLSelectAwait( const QL::TableQL& ql, UserPK userPK, SL sl )ι:
			TAwait<jvalue>{ sl },
			Query{ ql },
			UserPK{ userPK }
		{}
		α Suspend()ι->void;
	private:
		α GetStatement( const QL::TableQL& childTable, sp<DB::Column> joinColumn )ε->optional<DB::Statement>;
		α LoadRoles( const QL::TableQL& permissionRightsQL )ι->DB::RowAwait::Task;
		α LoadPermissionRights( const QL::TableQL& permissionRightsQL )ι->DB::RowAwait::Task;
		QL::TableQL Query;
		Jde::UserPK UserPK;
	};
	α AclQLSelectAwait::Suspend()ι->void{
		if( auto rights = Query.FindTable("permissionRights"); rights )
			LoadPermissionRights( *rights );
		else if( auto roles = Query.FindTable("roles"); roles )
			LoadRoles( *roles );
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
		}
		return statement;
	}
	α AclQLSelectAwait::LoadRoles( const QL::TableQL& roleQL )ι->DB::RowAwait::Task{
		jvalue y;
		try{
			if( auto statement = GetStatement(roleQL, GetTable("roles")->GetColumnPtr("role_id")); statement ){
				auto rows = co_await DS()->SelectCo( statement->Move() );
				let& columns = statement->Select.Columns;
				jarray j;
				for( auto& row : rows ){
					jobject entry;
					jobject* role{};
					for( uint i=0; i<row->Size() && i<columns.size(); ++i ){
						let& column = columns[i];
						auto& value = (*row)[i];
						let key = DB::Names::ToJson(column->Name);
						if( column->Table->Name=="roles" ){
							if( !role )
								role = &entry["role"].emplace_object();
							(*role)[key=="roleId" ? "id" : key] = value.Move();
						}
						else
							entry[key] = value.Move();
					}
					j.emplace_back( move(entry) );
				}
				y = Query.IsPlural()
					? move( j )
					: j.empty() ? jvalue{} : move(j.front());
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLSelectAwait::LoadPermissionRights( const QL::TableQL& permissionRights )ι->DB::RowAwait::Task{
		jvalue y;
		try{
			if( auto statement = GetStatement(permissionRights, GetTable("permission_rights")->GetColumnPtr("permission_id")); statement ){
				auto rows = co_await DS()->SelectCo( statement->Move() );
				let& columns = statement->Select.Columns;
				jarray j;
				for( auto& row : rows ){
					jobject entry;
					jobject* right{};
					jobject* resource{};
					for( uint i=0; i<row->Size() && i<columns.size(); ++i ){
						let& column = columns[i];
						auto& value = (*row)[i];
						let key = DB::Names::ToJson(column->Name);
						if( column->Table->Name=="permission_rights" || column->Table->Name=="resources" ){
							if( !right )
								right = &entry["permissionRight"].emplace_object();
							if( column->Table->Name=="permission_rights" )
								(*right)[key=="permissionId" ? "id" : key] = value.Move();
							else{
								if( !resource )
									resource = &(*right)["resource"].emplace_object();
								(*resource)[key] = value.Move();
							}
						}
						else
							entry[key] = value.Move();
					}
					j.emplace_back( move(entry) );
				}
				y = Query.IsPlural()
					? move(j)
					: j.empty() ? jvalue{} : move(j.front());
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α AclHook::Select( const QL::TableQL& ql, UserPK userPK, SL sl )ι->HookResult{
		return ql.DBName()=="acl"
			? mu<AclQLSelectAwait>( ql, userPK, sl )
			: nullptr;
	}

	α AclHook::InsertBefore( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="acl"
			? mu<AclQLAwait>( m, userPK, 0, sl )
			: nullptr;
	}
  α AclHook::InsertAfter( const QL::MutationQL& m, UserPK userPK, uint pk, SL sl )ι->HookResult{
		let& tableName = m.TableName();
		return tableName=="users" || tableName=="resources"
			? mu<AclQLAwait>( m, userPK, pk, sl )
			: nullptr;
	}
	α AclHook::UpdateAfter( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		let& tableName = m.TableName();
		return tableName=="permissions" || tableName=="resources" || tableName=="users" || tableName=="identityGroups" || tableName=="roles"
			? mu<AclQLAwait>( m, userPK, 0, sl )
			: nullptr;
	}
}