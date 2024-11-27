#include "Acl.h"
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/names.h>
#include <jde/ql/ql.h>
#include "Resource.h"

#define let const auto
namespace Jde::Access{
	α GetTable( str name )ε->sp<DB::View>;
	α AddAcl( IdentityPK identityPK, uint permissionPK, ERights allowed, ERights denied, ResourcePK resourcePK )ι->void;
	α AddAcl( IdentityPK identityPK, RolePK rolePK )ι->void;
	α AuthorizeAdmin( ResourcePK resourcePK, UserPK userPK, SL sl )ε->void;
	α UpdatePermission( PermissionPK permissionPK, optional<ERights> allowed, optional<ERights> denied )ε->void;
	α CreateResource( Resource&& args )ε->void;
	α UpdateResourceDeleted( str schemaName, const jobject& args, bool restored )ε->void;
	α CreateUser( IdentityPK identityPK )ι->void;
	α DeleteUser( IdentityPK identityPK )ι->void;
	α RestoreUser( IdentityPK identityPK )ι->void;

	α	DeleteRole( RolePK rolePK )ι->void;
	α RestoreRole( RolePK rolePK, flat_set<PermissionRole> members )ι->void;
	α AddToRole( RolePK rolePK, flat_set<PermissionRole> members )ι->void;
	α RemoveFromRole(	RolePK rolePK, flat_set<PermissionIdentityPK> toRemove )ι->void;

	α DeleteGroup( IdentityPK identityPK )ι->void;
	α RestoreGroup( IdentityPK identityPK, flat_set<IdentityPK>&& members )ι->void;
	α AddToGroup( GroupPK groupPK, flat_set<IdentityPK> members )ι->void;
	α RemoveFromGroup( GroupPK groupPK, flat_set<IdentityPK> members )ι->void;

	α LoadAcl( sp<DB::AppSchema> schema, AclLoadAwait& await )ι->DB::RowAwait::Task{
		try{
			sp<DB::Table> aclTable = schema->GetTablePtr( "acl" );
			let ds = aclTable->Schema->DS();
			auto statement = DB::SelectSKsSql( aclTable );
			let rows = co_await ds->SelectCo( move(statement) );
			flat_multimap<IdentityPK,PermissionRole> acl;
			for( let& row : rows ){
				let identityPK = row->GetUInt16(0);
				if( auto permissionPK = row->GetUIntOpt(1); permissionPK )
					acl.emplace( identityPK, PermissionRole{std::in_place_index<0>, *permissionPK} );
				else if( auto rolePK = row->GetUIntOpt(2); rolePK )
					acl.emplace( identityPK, PermissionRole{std::in_place_index<1>, *rolePK} );
			}
			await.Resume( move(acl) );
		}
		catch( IException& e ){
			await.ResumeExp( move(e) );
		}
	}

	void AclLoadAwait::Suspend()ι{
		LoadAcl( _schema, *this );
	}

	struct AclQLAwait final : TAwait<jvalue>{
		AclQLAwait( const QL::MutationQL& m, UserPK userPK, uint pk, SL sl )ι:
			TAwait<jvalue>{ sl },
			Mutation{ m },
			PK{ pk },
			UserPK{ userPK }
		{}
		α Suspend()ι->void override;
		α InsertAcl()ι->Coroutine::Task;
		α RestoreGroup( uint pk )ι->Coroutine::Task;
		α RestoreRole( uint pk )ι->DB::RowAwait::Task;
		α AddToRole( RolePK rolePK, flat_set<PermissionIdentityPK> members )ι->DB::MapAwait<PermissionIdentityPK,bool>::Task;
		QL::MutationQL Mutation;
		uint PK;
		Access::UserPK UserPK;
	private:
		//α Select()ι->DB::RowAwait::Task;
	};

	α AclQLAwait::InsertAcl()ι->Coroutine::Task{
		let& input = Mutation.Input();
		try{
			let table = GetTable( "acl" );
			let identityPK = Json::AsNumber<IdentityPK>( input, "identityId" );
			jobject y;
			if( auto p = input.find("permission"); p!=input.end() ){ //identityId:x, permission:{ allowed:x, denied:x, resource:{id:x} }
				let& permission = Json::AsObject( p->value() );
				let allowed = (ERights)Json::FindNumber<uint8>( permission, "allowed" ).value_or(0);
				let denied = (ERights)Json::FindNumber<uint8>( permission, "denied" ).value_or(0);
				let resourcePK = Json::AsNumber<ResourcePK>( permission, "resource/id" );
				AuthorizeAdmin( resourcePK, UserPK, _sl );
				DB::InsertClause insert{ table->UpsertProcName()+"_permission" };
				insert.Add( identityPK );

				insert.Add( underlying(allowed) );
				insert.Add( underlying(denied) );
				insert.Add( resourcePK );
				let sql = insert.Move();
				PermissionPK permissionPK;
				( co_await *table->Schema->DS()->ExecuteProcCo( sql.Text, move(sql.Params), [&](auto& row){
					permissionPK=row.GetUInt32(0);
				}) ).CheckError();
				y["permission"].emplace_object()["id"] = permissionPK;
				AddAcl( identityPK, permissionPK, allowed, denied, resourcePK );
			}
			else if( auto r = input.find("role"); r!=input.end() ){ //identityId:x, role:{ id:x }
				DB::InsertClause insert{ table->InsertProcName()+"_role" };
				insert.Add( identityPK );
				let rolePK = Json::AsNumber<ResourcePK>( input, "role/id" );
				insert.Add( rolePK );
				let sql = insert.Move();
				( co_await *table->Schema->DS()->ExecuteProcCo(sql.Text, move(sql.Params)) ).CheckError();
				AddAcl( identityPK, rolePK );
			}
			y["complete"]=true;
			Resume( jvalue{y} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α AclQLAwait::RestoreGroup( uint pk )ι->Coroutine::Task{
		auto table = GetTable( "identity_groups" );
		auto memberCol = table->GetColumnPtr( "member_id" );
		DB::Statement statement{ {{table->GetColumnPtr("identity_id"), memberCol}}, {table}, {memberCol, DB::Value{pk}} };
		try{
			auto userGroups = ( co_await table->Schema->DS()->template SelectSet<IdentityPK>(statement.Move()) ).UP<flat_set<IdentityPK>>();
			Access::RestoreGroup( pk, move(*userGroups) );
			Resume( jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α AclQLAwait::RestoreRole( uint pk )ι->DB::RowAwait::Task{
		let membersTable = GetTable( "role_members" );
		let idTable = GetTable( "permissions" );
		let roleCol = membersTable->GetColumnPtr( "role_id" );
		let memberCol = membersTable->GetColumnPtr( "member_id" );
		DB::FromClause from;
		from+=DB::Join{ memberCol, idTable->GetColumnPtr("permission_id"), true };
		DB::Statement statement{ {{memberCol, idTable->GetColumnPtr("is_role")}}, move(from), {roleCol, DB::Value{pk}} };
		try{
			flat_set<PermissionRole> members;
			auto rows = co_await idTable->Schema->DS()->SelectCo( statement.Move() );
			for( let& row : rows ){
				if( row->GetBit(1) )
					members.emplace( PermissionRole{std::in_place_index<1>, row->GetUInt32(0)} );
				else
					members.emplace( PermissionRole{std::in_place_index<0>, row->GetUInt32(0)} );
			}
			Access::RestoreRole( pk, move(members) );
			Resume( jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::AddToRole( RolePK rolePK, flat_set<PermissionIdentityPK> members )ι->DB::MapAwait<PermissionIdentityPK,bool>::Task{
		auto table = GetTable( "permissions" );
		try{
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
			Access::AddToRole( rolePK, move(members) );
			Resume( jvalue{} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

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
					CreateUser( PK );
				else if( Mutation.Type==Delete )
					DeleteUser( Mutation.Id() );
				else if( Mutation.Type==Restore )
					RestoreUser( Mutation.Id() );
			}
			else if( table->Name=="identityGroups" ){
				switch( Mutation.Type ){
				case Purge:
				case Delete:
					DeleteGroup( PK );
					break;
				case Restore:
					RestoreGroup( PK );
					break;
				case Add:
				case Remove:{
					auto members = Json::AsArray( Mutation.Args, "members" );
					flat_set<IdentityPK> memberPKs; memberPKs.reserve( members.size() );
					for( let& member : members )
						memberPKs.emplace( Json::AsNumber<IdentityPK>(member) );
					if( Mutation.Type==Add )
						AddToGroup( PK, move(memberPKs) );
					else if( Mutation.Type==Remove )
						RemoveFromGroup( PK, move(memberPKs) );
					break;}
					default: break;
				}
			}
			else if( table->Name=="roles" ){
				switch( Mutation.Type ){
				case Purge:
				case Delete:
					DeleteRole( PK );
					break;
				case Restore:
					RestoreRole( PK );
					return;
				case Add:
				case Remove:{
					auto members = Json::AsArray( Mutation.Args, "members" );
					flat_set<IdentityPK> memberPKs; memberPKs.reserve( members.size() );
					for( let& member : members )
						memberPKs.emplace( Json::AsNumber<IdentityPK>(member) );
					if( Mutation.Type==Add ){
						AddToRole( PK, move(memberPKs) );
						return;
					}
					else if( Mutation.Type==Remove )
						RemoveFromRole( PK, move(memberPKs) );
					break;}
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
		α Suspend()ι->void{ Load(); }
	private:
		α Load()ι->DB::RowAwait::Task;
		QL::TableQL Query;
		Access::UserPK UserPK;
	};
	α AclQLSelectAwait::Load()ι->DB::RowAwait::Task{
		try{
			jvalue y;
			let table = GetTable( "acl" );
			if( let rights = Query.FindTable("permissionRights"); rights ){
				let rightsTable = GetTable( "permission_rights" );
				auto statement = QL::SelectStatement( *rights );
				Trace{ ELogTags::Access, "{}", statement->Select.ToString() };
				if( auto aclStatement = !statement ? optional<DB::Statement>{} : QL::SelectStatement(Query); aclStatement ){
					statement->Select += move(aclStatement->Select);
					statement->Where += aclStatement->Where;
					statement->From += { rightsTable->FindColumn("permission_id"), table->FindColumn("permission_id"), true };
				}
				if( statement ){
					auto rows = co_await table->Schema->DS()->SelectCo( statement->Move() );
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