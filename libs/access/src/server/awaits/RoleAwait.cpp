#include <jde/access/server/awaits/RoleAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/names.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/QLAwait.h>
#include <jde/ql/types/TableQL.h>
#include <jde/access/Authorize.h>
#include "../serverInternal.h"
#include "../../accessInternal.h"
#pragma GCC diagnostic ignored "-Wdangling-reference"

#define let const auto
namespace Jde::Access::Server{

	//{ mutation addRole( id:42, allowed:255, denied:0, resource:{target:"users"} ) }
	//{ mutation addRole( id:11, role:{id:13} ) }
	α RoleMAwait::Add()ι->void{
		let rolePK = _mutation.Id<RolePK>();
		let args = _mutation.ExtrapolateVariables();
		if( auto role = args.find("role"); role!=args.end() )
			AddRole( rolePK, Json::AsObject(role->value()) );
		else if( auto rights = args.find("permissionRight"); rights!=args.end() )
			AddPermission( rolePK, Json::AsObject(rights->value()) );
		else
			ResumeExp( Exception{"Invalid mutation, expecting 'role' or 'permissionRight'."} );
	}
	α RoleMAwait::AddRole( RolePK parentRolePK, const jobject& childRole )ι->DB::ExecuteAwait::Task{
		try{
			let& table = GetTable( "role_members" );
			uint rowCount{};
			for( auto childRolePK : Json::ToVector<RolePK>(Json::AsValue(childRole, "id")) ){
				Authorizer().TestAddRoleMember( parentRolePK, childRolePK );
				DB::InsertClause insert;
				insert.Add( table.GetColumnPtr("role_id"), parentRolePK );
				insert.Add( table.GetColumnPtr("member_id"), childRolePK );
				rowCount += co_await table.Schema->DS()->Execute( insert.Move() );
			}
			QL::Subscriptions::OnMutation( _mutation, jvalue{} );
			Resume( rowCount );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α RoleMAwait::AddPermission( RolePK rolePK, const jobject& rights )ι->TAwait<PermissionRightsPK>::Task{
		try{
			//addRole( id:1, permissionRight:{allowed:1, denied:0, resource:{schema:\"opc.default\", target:\"nodeIds\", criteria:null}} )","variables":{}}
			let resource = Json::AsObject( rights, "resource" );
			auto schema = Json::FindString(resource, "schemaName");
			auto criteria = Json::FindString(resource, "criteria");
			auto resourceName = Json::FindString(resource, "name");
			let resourceTarget = Json::AsString(resource, "target");
			auto& auth = Authorizer();
			if( !schema )
				schema = auth.FindSchema( resourceTarget, _sl );
			auth.TestAdmin( *schema, resourceTarget, criteria.value_or(""), _userPK );
			let& table = GetTable( "roles" );
			DB::InsertClause insert{ DB::Names::ToSingular(table.DBName)+"_add" };
			insert.Add( rolePK );
			insert.Add( Json::FindNumber<uint8>(rights, "allowed").value_or(0) );
			insert.Add( Json::FindNumber<uint8>(rights, "denied").value_or(0) );
			insert.Add( resourceTarget );
			insert.Add( *schema );
			insert.AddOpt( move(resourceName) );
			insert.AddOpt( criteria );
			auto ds = table.Schema->DS();
			let permissionPK = co_await ds->InsertSeq<PermissionRightsPK>( move(insert) );
			jobject y;
			auto& permissionRight = y["permissionRight"].emplace_object();
			if( criteria ){
				auto resourcePK = auth.FindResourcePK(*schema, resourceTarget, *criteria);
				if( !resourcePK ){
					resourcePK = co_await ds->Scaler<ResourcePK>({
						Ƒ( "select resource_id from {} where schema_name=? and target=? and criteria=?", GetTable("resources").DBName ),
						{ {*schema}, {resourceTarget}, {*criteria} }
					 });
					auth.AddResource( *resourcePK, *schema, resourceTarget, *criteria );
				}
				permissionRight["resource"].emplace_object()["id"] = *resourcePK;
			}
			permissionRight["id"] = permissionPK;
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( y );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α RoleMAwait::Remove()ι->DB::ExecuteAwait::Task{ //removeRole( id:42, permissionRight:{id:420} )
		let& table = GetTable( "roles" );
		DB::InsertClause remove{ DB::Names::ToSingular(table.DBName)+"_remove" };
		remove.Add( _mutation.Id<RolePK>() );
		remove.Add( _mutation.AsPathNumber<PermissionPK>("permissionRight/id") );
		let y = co_await table.Schema->DS()->Execute( remove.Move() );
		QL::Subscriptions::OnMutation( _mutation, jvalue{} );
		ResumeScaler( y );
	}

	RoleAwait::RoleAwait( const QL::TableQL& q, Jde::UserPK userPK, SL sl )ε:
		TAwait<jvalue>{ sl },
		MemberTable{ GetTablePtr("role_members") },
		Query{ q },
		UserPK{ userPK }
	{}

	α RoleAwait::RoleStatement( QL::TableQL& roleQL )ε->optional<DB::Statement>{ //role( id:11 ){ role(id:13){id target deleted} }
		auto statement = QL::SelectStatement( roleQL, true );
		if( statement ){
			let& roleTable = GetTable( "roles" );
			statement->From = { {MemberTable->GetColumnPtr("member_id"), roleTable.GetPK(), true} };
			let memberRoleIdCol = MemberTable->GetColumnPtr( "role_id" );
			if( auto roleKey = Query.FindKey(); roleKey ){
				if( roleKey->IsPK() )
					statement->Where.Add( memberRoleIdCol, DB::Value::FromKey(*roleKey) );//role_members.role_id=?
				else{
					const string alias = "parent";
					statement->Where.Add( Ƒ("{}.target=?", alias) );
					statement->Where.Params().push_back( DB::Value::FromKey(*roleKey) );
					statement->From+={ MemberTable->GetSK0(), {}, roleTable.GetPK(), alias, true };
				}
			}
			statement->Select+=memberRoleIdCol;
			roleQL.Columns.push_back( QL::ColumnQL{"parentRoleId", memberRoleIdCol} );
		}
		return statement;
	}

	α RoleAwait::PermissionsStatement( QL::TableQL& permissionQL )ε->optional<DB::Statement>{
		auto permissionStatement = QL::SelectStatement( permissionQL, true );
		if( permissionStatement ){
			let& permissionsTable = GetTable( "permission_rights" );
			permissionStatement->From = { {MemberTable->GetColumnPtr("member_id"), permissionsTable.GetPK(), true} };
			permissionStatement->From +={ permissionsTable.GetColumnPtr("resource_id"), GetTable("resources").GetPK(), true };
			let rolePKCol = MemberTable->GetColumnPtr( "role_id" );
			if( auto roleKey = Query.FindKey(); roleKey ){
				if( roleKey->IsPK() )
					permissionStatement->Where.Add( rolePKCol, DB::Value::FromKey(*roleKey) );
				else{
					auto rolesTable = GetTable( "roles" );
					permissionStatement->Where.Add( rolesTable.GetColumnPtr("target"), DB::Value::FromKey(*roleKey) );
					permissionStatement->From += {MemberTable->GetSK0(), rolesTable.GetPK() };
				}
			}

			if( !permissionQL.FindColumn( "id" ) ){
				permissionStatement->Select+=permissionsTable.GetPK();
				permissionQL.Columns.push_back( QL::ColumnQL{"id", permissionsTable.GetPK()} );
			}
			permissionStatement->Select+=rolePKCol;
			permissionQL.Columns.push_back( QL::ColumnQL{"parentRoleId", rolePKCol} );
		}
		return permissionStatement;
	}

	//query{ role( id:42 ){permissionRights{id allowed denied resource(target:"users",criteria:null)}} }
	α RoleAwait::Select()ι->QL::QLAwait<>::Task{
		try{
			optional<jvalue> permissions;
			optional<jvalue> roleMembers;
			string permissionsKey = "permissionRights";
			string roleKey = "roles";
			// QL: role( id:16 ){permissionRight{id allowed denied resource(target:"users",criteria:null)} }
			if( auto permissionQL = Query.ExtractTable("permissionRights"); permissionQL ){
				if( auto statement = PermissionsStatement( *permissionQL ); statement ){
					auto rights = co_await QL::QLAwait<jvalue>{ move(*permissionQL), move(*statement), UserPK };
					if( rights.is_array() )
						permissions = move( rights.get_array() );
					else if( rights.is_object() ){
						permissionsKey = "permissionRight";
						permissions = move( rights.get_object() );
					}
				}
			}
			if( auto roleQL = Query.ExtractTable("roles"); roleQL ){
				if( auto statement = RoleStatement(*roleQL); statement ){
					auto dbMembers = co_await QL::QLAwait( move(*roleQL), move(*statement), UserPK );
					if( dbMembers.is_array() )
						roleMembers = dbMembers.get_array().empty() ? jarray{} : move( dbMembers.get_array() );
					else if( dbMembers.is_object() ){
						roleKey = "role";
						roleMembers = dbMembers.get_object().empty() ? jobject{} : move( dbMembers.get_object() );
					}
				}
			}

			flat_map<RolePK,jobject> roles;
			auto createRolesFromMembers = [&roles]( jvalue& roleMembers, str memberName ){
				auto addRoleMember = [&]( jobject& member ){
					let parentRolePK = Json::AsNumber<RolePK>( member, "parentRoleId" );
					member.erase( "parentRoleId" );
					auto role = roles.try_emplace( parentRolePK, jobject{ {"id", parentRolePK} } );
					auto& jmember = role.first->second;
					if( role.second || !jmember.contains(memberName) ){//first row
						if( roleMembers.is_array() ){
							jmember[memberName] = jarray{move(member)};
						}else
							jmember[DB::Names::ToSingular(memberName)] = move( member );
					}
					else if( roleMembers.is_array() )
						jmember[memberName].get_array().emplace_back( move(member) );
				};
				Json::Visit( roleMembers, addRoleMember );
			};

			if( permissions )
				createRolesFromMembers( *permissions, "permissionRights" );
			if( roleMembers )
				createRolesFromMembers( *roleMembers, "roles" );
			let& roleTable = GetTable( "roles" );
			if( !Query.FindColumn("id") )
				Query.Columns.push_back( QL::ColumnQL{"id", roleTable.GetPK()} );
			let returnArray = Query.IsPlural();
			if( auto roleStatement = QL::SelectStatement( Query ); roleStatement ){
				Query.ReturnRaw = true;
				auto qlRoles = co_await QL::QLAwait( move(Query), UserPK );
				auto addRole = [&]( jobject&& roleProperties ){
					let rolePK = Json::AsNumber<RolePK>( roleProperties, "id" );
					auto existing = roles.find( rolePK );
					if( existing!=roles.end() ){
						for( auto&& [key,value] : roleProperties )
							existing->second[key] = move( value );
					}else
						existing = roles.emplace( rolePK, move(roleProperties) ).first;
					auto& role = existing->second;
					if( permissions && !role.contains(permissionsKey) )
						role["permissionRights"] = permissions->is_array() ? (jvalue)jarray{} : (jvalue)jobject{};
					if( roleMembers && !role.contains(roleKey) )
						role["roles"] = roleMembers->is_array() ? (jvalue)jarray{} : (jvalue)jobject{};
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
}