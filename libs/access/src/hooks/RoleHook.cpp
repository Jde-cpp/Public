#include "RoleHook.h"
#include <jde/access/Authorize.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/types/TableQL.h>
#include "../accessInternal.h"

#define let const auto
namespace Jde::Access{
	α GetTable( str name )ε->sp<DB::View>;

	struct RoleMutationAwait final : TAwait<jvalue>{
		RoleMutationAwait( const QL::MutationQL& m, UserPK userPK, SL sl )ι:TAwait<jvalue>{ sl }, Mutation{m}, _userPK{userPK}{}
		α Suspend()ι->void override{ if(Mutation.Type==QL::EMutationQL::Remove) Remove(); else Add(); }
	private:
		α Add()ι->void;
		α AddRole( RolePK parentRolePK, const jobject& childRole )ι->TAwait<uint>::Task;
		α AddPermission( RolePK parentRolePK, const jobject& permissionRights )ι->TAwait<PermissionPK>::Task;
		α Remove()ι->TAwait<PermissionPK>::Task;

		QL::MutationQL Mutation;
		UserPK _userPK;
	};

	//{ mutation addRole( id:42, allowed:255, denied:0, resource:{target:"users"} ) }
	//{ mutation addRole( id:11, role:{id:13} ) }
	α RoleMutationAwait::Add()ι->void{
		let rolePK = Mutation.Id<RolePK>();
		if( auto role = Mutation.Args.find("role"); role!=Mutation.Args.end() )
			AddRole( rolePK, Json::AsObject(role->value()) );
		else if( auto rights = Mutation.Args.find("permissionRight"); rights!=Mutation.Args.end() )
			AddPermission( rolePK, Json::AsObject(rights->value()) );
		else
			ResumeExp( Exception{"Invalid mutation."} );
	}
	α RoleMutationAwait::AddRole( RolePK parentRolePK, const jobject& childRole )ι->TAwait<uint>::Task{
		try{
			let childRolePK = Json::AsNumber<RolePK>(childRole, "id");
			Authorizer().TestAddRoleMember( parentRolePK, childRolePK );
			let table = GetTable( "role_members" );
			DB::InsertClause insert;
			insert.Add( table->GetColumnPtr("role_id"), parentRolePK );
			insert.Add( table->GetColumnPtr("member_id"), childRolePK );
			let rowCount = co_await table->Schema->DS()->ExecuteCo( insert.Move() );
			Resume( rowCount );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α RoleMutationAwait::AddPermission( RolePK rolePK, const jobject& rights )ι->TAwait<PermissionPK>::Task{
		try{
			const string resource{ Json::AsSVPath(rights, "resource/target") };
			Authorizer().TestAdmin( resource, _userPK );
			let table = GetTable( "roles" );
			DB::InsertClause insert{ DB::Names::ToSingular(table->DBName)+"_add" };
			insert.Add( rolePK );
			insert.Add( Json::FindNumber<uint8>(rights, "allowed").value_or(0) );
			insert.Add( Json::FindNumber<uint8>(rights, "denied").value_or(0) );
			insert.Add( resource );
			let permissionPK = co_await table->Schema->DS()->ExecuteScaler<PermissionRightsPK>( insert.Move() );
			jobject y;
			y["permissionRight"].emplace_object()["id"] = permissionPK;
			Trace{ ELogTags::Test, "RoleMutationAwait::AddPermission - '{}'", serialize(y) };
			Resume( y );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α RoleMutationAwait::Remove()ι->TAwait<PermissionPK>::Task{ //removeRole( id:42, permissionRight:{id:420} )
		let table = GetTable( "roles" );
		DB::InsertClause remove{ DB::Names::ToSingular(table->DBName)+"_remove" };
		remove.Add( Mutation.Id<RolePK>() );
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
		Jde::UserPK UserPK;
	private:
		α PermissionsStatement( QL::TableQL& permissionQL )ε->optional<DB::Statement>;
		α RoleStatement( QL::TableQL& roleQL )ε->optional<DB::Statement>;
		α Select()ι->QL::QLAwait<>::Task;
	};

	α RoleSelectAwait::RoleStatement( QL::TableQL& roleQL )ε->optional<DB::Statement>{ //role( id:11 ){ role(id:13){id target deleted} }
		auto statement = QL::SelectStatement( roleQL, true );
		if( statement ){
			let memberRoleIdCol = MemberTable->GetColumnPtr( "role_id" );
			if( auto roleId = Query.Args.find("id"); roleId!=roleQL.Args.end() )
				statement->Where.Add( memberRoleIdCol, DB::Value{roleId->value().to_number<RolePK>()} );//role_members.role_id=?
			let& roleTable = *GetTable( "roles" );
			statement->Select+=memberRoleIdCol;
			roleQL.Columns.push_back( QL::ColumnQL{"parentRoleId", memberRoleIdCol} );
			statement->From+={ roleTable.GetPK(), MemberTable->GetColumnPtr("member_id"), true };
		}
		return statement;
	}

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
			permissionQL.Columns.push_back( QL::ColumnQL{"parentRoleId", rolePKCol} );
			permissionStatement->From+={ rightsTable.GetPK(), MemberTable->GetColumnPtr("member_id"), true };
		}
		return permissionStatement;
	}

	//query{ role( id:42 ){permissionRights{id allowed denied resource(target:"users",criteria:null)}} }
	α RoleSelectAwait::Select()ι->QL::QLAwait<>::Task{
		try{
			optional<jvalue> permissions;
			optional<jvalue> roleMembers;
			string memberName{};
			if( auto permissionQL = Query.FindTable("permissionRights"); permissionQL ){
				if( auto statement = PermissionsStatement( *permissionQL ); statement ){
					memberName = permissionQL->JsonName;
					let tableName = permissionQL->JsonName;
					auto rights = co_await QL::QLAwait( move(*permissionQL), move(*statement), UserPK );
					if( rights.is_array() )
						permissions = rights.get_array().empty() ? optional<jvalue>{} : move( rights.get_array() );
					else if( rights.is_object() )
						permissions = move( rights.get_object() );
				}
			}
			else if( auto roleQL = Query.FindTable("roles"); roleQL ){
				if( auto statement = RoleStatement(*roleQL); statement ){
					memberName = roleQL->JsonName;
					let tableName = roleQL->JsonName;
					auto dbMembers = co_await QL::QLAwait( move(*roleQL), move(*statement), UserPK );
					if( dbMembers.is_array() )
						roleMembers = dbMembers.get_array().empty() ? optional<jvalue>{} : move( dbMembers.get_array() );
					else if( dbMembers.is_object() )
						roleMembers = dbMembers.get_object().empty() ? optional<jvalue>{} : move( dbMembers.get_object() );
				}
			}

			flat_map<RolePK,jobject> roles;
			auto createRolesFromMembers = [&roles,&memberName]( jvalue&& roleMembers ){
				//let singularMember = DB::Names::IsPlural(memberName);
				auto addRoleMember = [&]( jobject&& member ){
					let parentRolePK = Json::AsNumber<RolePK>( member, "parentRoleId" );
					member.erase( "parentRoleId" );
					auto role = roles.try_emplace( parentRolePK, jobject{ {"id", parentRolePK} } );
					auto& jmember = role.first->second;
					if( role.second ){
						if( roleMembers.is_array() )
							jmember[memberName] = jarray{move(member)};
						else
							jmember[memberName] = move(member);
					}
					else if( !roleMembers.is_array() )
						jmember[memberName].get_array().emplace_back( move(member) );
				};
				Json::Visit( move(roleMembers), addRoleMember );
			};

			if( permissions )
				createRolesFromMembers( move(*permissions) );
			else if( roleMembers )
				createRolesFromMembers( move(*roleMembers) );

			let& roleTable = *GetTable( "roles" );
			if( !Query.FindColumn("id") )
				Query.Columns.push_back( QL::ColumnQL{"id", roleTable.GetPK()} );
			let returnArray = Query.IsPlural();
			if( auto roleStatement = QL::SelectStatement( Query ); roleStatement ){
				//Trace{ ELogTags::Test, "{}", Query.ToString() };
				//let roleMemberRolePKColumn = MemberTable->GetColumnPtr("role_id");
				//Query.Columns.push_back( QL::ColumnQL{"memberId", MemberTable->GetColumnPtr("member_id")} );
				//Query.Columns.push_back( QL::ColumnQL{"roleId", roleMemberRolePKColumn} );
				//roleStatement->From+={ roleTable.GetPK(), roleMemberRolePKColumn, true };
				auto qlRoles = co_await QL::QLAwait( move(Query), UserPK );
				auto addRole = [&roles]( jobject&& role ){
					let rolePK = Json::AsNumber<RolePK>( role, "id" );
					if( auto existing = roles.find(rolePK); existing!=roles.end() ){
						for( auto&& [key,value] : role )
							existing->second[key] = move( value );
					}else
						roles.emplace( rolePK, role );
				};
				Json::Visit( move(qlRoles), addRole );
			}
			jvalue y;
			if( returnArray ){
				jarray jRoles;
				for( auto&& [_,value] : roles )
					jRoles.emplace_back( move(value) );
				y = jRoles;
			}
			else if( roles.size() )
				y = move( roles.begin()->second );
			Resume( move(y) );
		}
		catch( boost::system::system_error& e ){
			ResumeExp( CodeException{e.code(), ELogTags::Access, ELogLevel::Debug} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α RoleHook::Select( const QL::TableQL& q, UserPK userPK, SL sl )ι->HookResult{
		return q.JsonName.starts_with("role") && (q.FindTable("roles") || q.FindTable("permissionRights"))
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