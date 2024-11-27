#include <jde/access/hooks/RoleHook.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/ql.h>

#define let const auto
namespace Jde::Access{
	α GetTable( str name )ε->sp<DB::View>;
	α AuthorizeAdmin( str resource, UserPK userPK, SL sl )ε->void;

	struct RoleMutationAwait final : TAwait<jvalue>{
		RoleMutationAwait( const QL::MutationQL& m, UserPK userPK, SL sl )ι:TAwait<jvalue>{ sl }, Mutation{m}, _userPK{userPK}{}
		α Suspend()ι->void override{ if(Mutation.Type==QL::EMutationQL::Remove) Remove(); else Add(); }
	private:
		α Add()ι->TAwait<PermissionPK>::Task;
		α Remove()ι->TAwait<PermissionPK>::Task;

		QL::MutationQL Mutation;
		UserPK _userPK;
	};

	//{ mutation addRole( id:42, allowed:255, denied:0, resource:{target:"users"} ) }
	α RoleMutationAwait::Add()ι->TAwait<PermissionPK>::Task{
		try{
			const string resource{ Json::AsSVPath(Mutation.Args, "resource/target") };
			AuthorizeAdmin( resource, _userPK, _sl );
			let table = GetTable( "roles" );
			DB::InsertClause insert{ DB::Names::ToSingular(table->DBName)+"_add" };
			insert.Add( Json::AsNumber<RolePK>(Mutation.Args, "id") );
			insert.Add( Json::FindNumber<uint8>(Mutation.Args, "allowed").value_or(0) );
			insert.Add( Json::FindNumber<uint8>(Mutation.Args, "denied").value_or(0) );
			insert.Add( resource );
			let permissionPK = co_await table->Schema->DS()->ExecuteScaler<PermissionPK>( insert.Move() );
			jobject y;
			y["id"] = permissionPK;
			Resume( y );
		}
		catch( Exception& e ){
			ResumeExp( move(e) );
		}
	}
	α RoleMutationAwait::Remove()ι->TAwait<PermissionPK>::Task{ //removeRole( id:42, permissionRight:{id:420} )
		let table = GetTable( "roles" );
		DB::InsertClause remove{ DB::Names::ToSingular(table->DBName)+"_remove" };
		remove.Add( Json::AsNumber<RolePK>(Mutation.Args, "id") );
		remove.Add( Json::AsNumber<PermissionPK>(Mutation.Args, "permissionRight/id") );
		let rowCount = co_await table->Schema->DS()->ExecuteProcCo( remove.Move() );
		Resume( rowCount );
	}

	struct RoleSelectAwait final : TAwait<jvalue>{
		RoleSelectAwait( const QL::TableQL& q, UserPK userPK, SRCE )ε:
			TAwait<jvalue>{ sl },
			MemberTable{ GetTable("role_members") },
			Query{ q },
			UserPK{ userPK }
		{}
		α Suspend()ι->void override{ Select(); }
		sp<DB::View> MemberTable;
		QL::TableQL Query;
		Access::UserPK UserPK;
	private:
		α PermissionsStatement( QL::TableQL& permissionQL )ε->optional<DB::Statement>;
		α Select()ι->QL::QLAwait::Task;
	};
	α RoleSelectAwait::PermissionsStatement( QL::TableQL& permissionQL )ε->optional<DB::Statement>{
		auto permissionStatement = QL::SelectStatement( permissionQL, true );
		if( permissionStatement ){
			let rolePKCol = MemberTable->GetColumnPtr( "role_id" );
			if( auto roleId = Query.Args.find("id"); roleId!=permissionQL.Args.end() )
				permissionStatement->Where.Add( rolePKCol, DB::Value{roleId->value().to_number<RolePK>()} );//role_id=?
			let& rightsTable = *GetTable( "permission_rights" );
			if( !permissionQL.FindColumn( "id" ) ){
				permissionStatement->Select+=rightsTable.GetPK();
				permissionQL.Columns.push_back( QL::ColumnQL{"id", rightsTable.GetPK()} );
			}
			permissionStatement->Select+=rolePKCol;
			permissionQL.Columns.push_back( QL::ColumnQL{"roleId", rolePKCol} );
			permissionStatement->From+={ rightsTable.GetPK(), MemberTable->GetColumnPtr("member_id"), true };
		}
		return permissionStatement;
	}
	//query{ role( id:42 ){permissionRights{id allowed denied resource(target:"users",criteria:null)}} }} }}
	α RoleSelectAwait::Select()ι->QL::QLAwait::Task{
		try{
			optional<jarray> permissions;
			bool singularRights{};
			if( auto permissionQL = find_if( Query.Tables, [](let& t){ return t.JsonName.starts_with("permissionRight"); } ); permissionQL!=Query.Tables.end() ){
				if( auto statement = PermissionsStatement( *permissionQL); statement ){
					singularRights = !permissionQL->IsPlural();
					let tableName = permissionQL->JsonName;
					auto rights = co_await QL::QLAwait( move(*permissionQL), move(*statement), UserPK, _sl );
					//Trace{ ELogTags::Access, "{}", serialize(rights) };
					if( rights.is_array() )
						permissions = rights.get_array().empty() ? optional<jarray>{} : move( rights.get_array() );
					else if( rights.is_object() )
						permissions = jarray{ move(rights.get_object()) };
				}
			}
			jarray roles;
			if( auto roleStatement = QL::SelectStatement( Query ); roleStatement ){
				let& roleTable = *GetTable( "roles" );
				let roleMemberRolePKColumn = MemberTable->GetColumnPtr("role_id");
				Query.Columns.push_back( QL::ColumnQL{"memberId", MemberTable->GetColumnPtr("member_id")} );
				Query.Columns.push_back( QL::ColumnQL{"roleId", roleMemberRolePKColumn} );
				roleStatement->From+={ roleTable.GetPK(), roleMemberRolePKColumn, true };
				let qlResult = co_await QL::QLAwait( move(Query), UserPK, _sl );
				if( !permissions ){
					Resume( jvalue{move(qlResult)} );
					co_return;
				}
				let qlRoles = Json::AsObject( qlResult ).at( Query.JsonName );
				if( qlRoles.is_object() )
					roles.push_back( move(qlRoles.get_object()) );
				else if( qlRoles.is_array() )
					roles = move( qlRoles.get_array() );
			}
			else if( !permissions ){
				co_return Resume( jvalue{} );
			}
			else{
				flat_set<RolePK> rolePKs;
				for( let& value : *permissions ){
					let rolePK = Json::AsNumber<RolePK>( Json::AsObject(value), "roleId" );
					if( rolePKs.emplace( rolePK ).second )
						roles.push_back( jobject{ {"id", rolePK} } );
				}
			}
			auto addPermissionRights = [&](jobject& role){
				let rolePK = Json::FindNumber<RolePK>( role, "id" );
				jarray rolePermissions;
				for( auto&& permissionValue : *permissions ){
					auto& permission = permissionValue.as_object();
					let memberRolePK = Json::AsNumber<RolePK>( permission, "roleId" );
					if( !rolePK || memberRolePK==rolePK ){
						permission.erase( "roleId" );
						rolePermissions.emplace_back( move(permission) );
					}
				}
				if( singularRights )
					role["permissionRight"] = rolePermissions.size() ? move(rolePermissions.front()) : jvalue{};
				else
					role["permissionRights"] = move( rolePermissions );
			};
			for( auto& role : roles )
				addPermissionRights( role.as_object() );
			Trace( ELogTags::Access, "{}", serialize(roles) );
			Resume( Query.IsPlural() ? move(roles) : roles.empty() ? jvalue{} : move(roles.front()) );
		}
		catch( boost::system::system_error& e ){
			ResumeExp( CodeException{e.code(), ELogTags::Access, ELogLevel::Debug} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α RoleHook::Select( const QL::TableQL& q, UserPK userPK, SL sl )ι->HookResult{
		return q.JsonName.starts_with("role") && find_if(q.Tables, [](let& t){ return t.JsonName.starts_with("permissionRight"); })!=q.Tables.end()
			? mu<RoleSelectAwait>( q, userPK, sl )
			: nullptr;
	}

	α RoleHook::Add( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="roles" ? mu<RoleMutationAwait>( m, userPK, sl ) : nullptr;
	}

	α RoleHook::Remove( const QL::MutationQL& m, UserPK userPK, SL sl )ι->HookResult{
		return m.TableName()=="roles" ? mu<RoleMutationAwait>( m, userPK, sl ) : nullptr;
	}
}
