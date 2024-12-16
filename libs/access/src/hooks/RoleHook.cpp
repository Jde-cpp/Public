#include <jde/access/hooks/RoleHook.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/ql.h>
#include "../Authorize.h"

#define let const auto
namespace Jde::Access{
	α GetTable( str name )ε->sp<DB::View>;
	α AuthorizeAdmin( str resource, UserPK userPK, SL sl )ε->void;
	α AddToRole( RolePK rolePK, PermissionPK member, ERights allowed, ERights denied, str resource )ι->void;
	α AddToRole( RolePK parentRolePK, RolePK childRolePK )ι->void;

	struct RoleMutationAwait final : TAwait<jvalue>{
		RoleMutationAwait( const QL::MutationQL& m, UserPK userPK, SL sl )ι:TAwait<jvalue>{ sl }, Mutation{m}, _userPK{userPK}{}
		α Suspend()ι->void override{ if(Mutation.Type==QL::EMutationQL::Remove) Remove(); else Add(); }
	private:
		α Add()ι->void;
		α AddRole( RolePK parentRolePK, const jobject& childRole )ι->TAwait<uint>::Task;
		α AddPermission( const jobject& resource )ι->TAwait<PermissionPK>::Task;
		α Remove()ι->TAwait<PermissionPK>::Task;

		QL::MutationQL Mutation;
		UserPK _userPK;
	};

	//{ mutation addRole( id:42, allowed:255, denied:0, resource:{target:"users"} ) }
	//{ mutation addRole( id:11, role:{id:13} ) }
	α RoleMutationAwait::Add()ι->void{
		if( auto role = Mutation.Args.find("role"); role!=Mutation.Args.end() )
			AddRole( Mutation.Id<RolePK>(), Json::AsObject(role->value()) );
		else if( auto resource = Mutation.Args.find("resource"); resource!=Mutation.Args.end() )
			AddPermission( Json::AsObject(resource->value()) );
		else
			ResumeExp( Exception{"Invalid mutation."} );
	}
	α RoleMutationAwait::AddRole( RolePK parentRolePK, const jobject& childRole )ι->TAwait<uint>::Task{
		try{
			let childRolePK = Json::AsNumber<RolePK>(childRole, "id");
			Authorizer().TestAddRoleMember( parentRolePK, childRolePK, _sl );
			let table = GetTable( "role_members" );
			DB::InsertClause insert;
			insert.Add( table->GetColumnPtr("role_id"), parentRolePK );
			insert.Add( table->GetColumnPtr("member_id"), childRolePK );
			let rowCount = co_await table->Schema->DS()->ExecuteCo( insert.Move() );
			AddToRole( parentRolePK, childRolePK );
			Resume( rowCount );
		}
		catch( Exception& e ){
			ResumeExp( move(e) );
		}
	}
	α RoleMutationAwait::AddPermission( const jobject& resource )ι->TAwait<PermissionPK>::Task{
		try{
			const string resource{ Json::AsSVPath(Mutation.Args, "resource/target") };
			AuthorizeAdmin( resource, _userPK, _sl );
			let table = GetTable( "roles" );
			DB::InsertClause insert{ DB::Names::ToSingular(table->DBName)+"_add" };
			let rolePK = Json::AsNumber<RolePK>(Mutation.Args, "id");
			let allowed = Json::FindNumber<uint8>(Mutation.Args, "allowed").value_or(0);
			let denied = Json::FindNumber<uint8>(Mutation.Args, "denied").value_or(0);
			insert.Add( rolePK );
			insert.Add( allowed );
			insert.Add( denied );
			insert.Add( resource );
			let permissionPK = co_await table->Schema->DS()->ExecuteScaler<PermissionPK>( insert.Move() );
			AddToRole( rolePK, permissionPK, (ERights)allowed, (ERights)denied, resource );
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
		Jde::UserPK UserPK;
	private:
		α PermissionsStatement( QL::TableQL& permissionQL )ε->optional<DB::Statement>;
		α RoleStatement( QL::TableQL& roleQL )ε->optional<DB::Statement>;
		α Select()ι->QL::QLAwait::Task;
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
			statement->From+={ roleTable.GetPK(), MemberTable->GetColumnPtr("member_id"), false };
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
	//query{ role( id:42 ){permissionRights{id allowed denied resource(target:"users",criteria:null)}} }} }}
	α RoleSelectAwait::Select()ι->QL::QLAwait::Task{
		try{
			optional<jvalue> permissions;
			optional<jvalue> roleMembers;
			string memberName{};
			if( auto permissionQL = find_if( Query.Tables, [](let& t){ return t.JsonName.starts_with("permissionRight"); } ); permissionQL!=Query.Tables.end() ){
				if( auto statement = PermissionsStatement( *permissionQL); statement ){
					memberName = permissionQL->JsonName;
					let tableName = permissionQL->JsonName;
					auto rights = co_await QL::QLAwait( move(*permissionQL), move(*statement), UserPK, _sl );
					if( rights.is_array() )
						permissions = rights.get_array().empty() ? optional<jvalue>{} : move( rights.get_array() );
					else if( rights.is_object() )
						permissions = move( rights.get_object() );
				}
			}
			else if( auto roleQL = find_if(Query.Tables, [](let& t){ return t.JsonName.starts_with("role"); }); roleQL!=Query.Tables.end() ){
				if( auto statement = RoleStatement( *roleQL); statement ){
					memberName = roleQL->JsonName;
					let tableName = roleQL->JsonName;
					auto dbMembers = co_await QL::QLAwait( move(*roleQL), move(*statement), UserPK, _sl );
					if( dbMembers.is_array() )
						roleMembers = dbMembers.get_array().empty() ? optional<jvalue>{} : move( dbMembers.get_array() );
					else if( dbMembers.is_object() )
						roleMembers = dbMembers.get_object().empty() ? optional<jvalue>{} : move( dbMembers.get_object() );
				}
			}

			flat_map<RolePK,jobject> roles;
			auto createRolesFromMembers = [&roles,&memberName]( jvalue&& roleMembers ){
				//let singularMember = DB::Names::IsPlural(memberName);
				auto addRoleMember = [&]( jobject&& member, bool array ){
					let parentRolePK = Json::AsNumber<RolePK>( member, "parentRoleId" );
					auto role = roles.try_emplace( parentRolePK, jobject{ {"id", parentRolePK} } );
					auto& jmember = role.first->second;
					if( role.second ){
						if( array )
							jmember[memberName] = jarray{move(member)};
						else
							jmember[memberName] = move(member);
					}
					else if( !array )
						jmember[memberName].get_array().emplace_back( move(member) );
				};
				if( roleMembers.is_array() ){
					for( auto&& member : roleMembers.get_array() )
						addRoleMember( move(member.as_object()), true );
				}
				else if( roleMembers.is_object() )
					addRoleMember( move(roleMembers.as_object()), false );
			};

			if( permissions )
				createRolesFromMembers( move(*permissions) );
			else if( roleMembers )
				createRolesFromMembers( move(*roleMembers) );
			if( auto roleStatement = QL::SelectStatement( Query ); roleStatement ){
				let& roleTable = *GetTable( "roles" );
				let roleMemberRolePKColumn = MemberTable->GetColumnPtr("role_id");
				Query.Columns.push_back( QL::ColumnQL{"memberId", MemberTable->GetColumnPtr("member_id")} );
				Query.Columns.push_back( QL::ColumnQL{"roleId", roleMemberRolePKColumn} );
				roleStatement->From+={ roleTable.GetPK(), roleMemberRolePKColumn, true };
				auto qlResult = co_await QL::QLAwait( move(Query), UserPK, _sl );
				auto qlRoles = qlResult.as_object().at( Query.JsonName );

				auto addRole = [&roles]( jobject&& role ){
					let rolePK = Json::AsNumber<RolePK>( role, "id" );
					if( auto existing = roles.find(rolePK); existing!=roles.end() ){
						for( auto&& [key,value] : role )
							existing->second[key] = move( value );
					}else
						roles.emplace( rolePK, role );
				};
				if( qlRoles.is_object() )
					addRole( move(qlRoles.get_object()) );
				else if( qlRoles.is_array() ){
					for( auto&& role : qlRoles.get_array() )
						addRole( move(role.get_object()) );
				}
			}
			jvalue y;
			if( roles.size() ){
				if( Query.IsPlural() ){
					jarray jRoles;
					for( auto&& [_,value] : roles )
						jRoles.emplace_back( move(value) );
					y = jRoles;
				}
				else
					y = move( roles.begin()->second );
			}
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
		return q.JsonName.starts_with("role") && find_if(q.Tables, [](let& t){ return t.JsonName.starts_with("role") || t.JsonName.starts_with("permissionRight"); })!=q.Tables.end()
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
