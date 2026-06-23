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

#define let const auto
namespace Jde::Access::Server{

	//{ mutation addRole(id:42, allowed:255, denied:0, resource:{target:"users"}) }
	//{ mutation addRole(id:11, role:{id:13}) }
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
			auto& resource = Json::AsObject( rights, "resource" );
			auto schema = Json::FindString( resource, "schemaName" );
			auto criteria = Json::FindString( resource, "criteria" );
			if( criteria && criteria->empty() )
				criteria = nullopt;
			auto resourceName = Json::FindString( resource, "name" );
			auto& auth = Authorizer();
			auto resourceKey = Json::AsKey( resource );
			if( resourceKey.IsPK() ){
				if( auto existing = auth.FindResource( Resource{resourceKey.PK(), {}} ); existing ){
					resourceKey = DB::Key{ existing->Target };
					schema = existing->Schema;
				}else
					THROW( "Resource with PK '{}' not found.", resourceKey.PK() );
			}
			if( !schema )
				schema = auth.GetSchema( resourceKey.NK(), _sl );

			auth.TestAdmin( *schema, resourceKey.NK(), criteria.value_or(""), _userPK );
			let& table = GetTable( "roles" );
			DB::InsertClause insert{ DB::Names::ToSingular(table.DBName)+"_add" };
			insert.Add( rolePK );
			insert.Add( Json::FindNumber<uint>(rights, "allowed").value_or(0) );
			insert.Add( Json::FindNumber<uint>(rights, "denied").value_or(0) );
			insert.Add( resourceKey.NK() );
			insert.Add( *schema );
			insert.AddOpt( move(resourceName) );
			insert.AddOpt( criteria );
			auto ds = table.Schema->DS();
			let permissionPK = co_await ds->InsertSeq<PermissionRightsPK>( move(insert) );
			jobject y;
			auto& permissionRight = y["permissionRight"].emplace_object();
			//if( criteria ){
				auto resourcePK = auth.FindResourcePK( *schema, resourceKey.NK(), criteria.value_or(string{}) );
				vector<DB::Value> params{ {*schema}, {resourceKey.NK()} };
				if( !resourcePK ){
					string dbCriteria;
					if( criteria.has_value() ){
						dbCriteria = "criteria=?";
						params.emplace_back( *criteria );
					}
					else
						dbCriteria = "criteria is null";
					let foundPK = co_await ds->Scaler<ResourcePK>({ //COALESCE so a missing/deleted resource returns 0 instead of throwing "No value returned".
						Ƒ( "select coalesce( (select resource_id from {} where schema_name=? and target=? and {} and deleted is null), 0 )", GetTable("resources").DBName, dbCriteria ),
						move(params)
					});
					if( foundPK ){
						resourcePK = foundPK;
						auth.AddResource( *resourcePK, *schema, resourceKey.NK(), criteria.value_or(string{}) );
					}
				}
				if( resourcePK )
					permissionRight["resource"].emplace_object()["id"] = *resourcePK;
			//}
			permissionRight["id"] = permissionPK;
			QL::Subscriptions::OnMutation(
				_mutation,
				y,
				[&]( QL::TableQL& subscription )->bool {
					let permissionRights = subscription.FindTable("permissionRights");
					if( !permissionRights )
						return true;
					let resource = permissionRights->FindTable("resource");
					if( !resource )
						return true;
					let subscriptionSchema = resource->FindPtr<jvalue>("schemaName");
					if( !subscriptionSchema )
						return true;
					if( subscriptionSchema->is_string() )
						return *schema==subscriptionSchema->as_string();
					else if( subscriptionSchema->is_array() ){
						for( let& v : subscriptionSchema->as_array() ){
							if( v.is_string() && *schema==v.get_string() )
								return true;
						}
						return false;
					}
					DBGT( ELogTags::Access, "Unexpected schemaName type in subscription: {}", serialize(*subscriptionSchema) );
					return false;
				}
			);
			Resume( y );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α RoleMAwait::Remove()ι->DB::ExecuteAwait::Task{ //removeRole(id:42, permissionRight:{id:420})
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

	α RoleAwait::RoleStatement( QL::TableQL& roleQL )ε->optional<DB::Statement>{ //role(id:11){role(id:13){id target deleted}}
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
					permissionStatement->From += { MemberTable->GetSK0(), rolesTable.GetPK() };
				}
			}

			if( !permissionQL.FindColumn("id") ){
				permissionStatement->Select+=permissionsTable.GetPK();
				permissionQL.Columns.push_back( QL::ColumnQL{"id", permissionsTable.GetPK()} );
			}
			permissionStatement->Select+=rolePKCol;
			permissionQL.Columns.push_back( QL::ColumnQL{"parentRoleId", rolePKCol} );
		}
		return permissionStatement;
	}

	//query{ role(id:42){permissionRights{id allowed denied resource(target:"users",criteria:null)}} }
	α RoleAwait::Select()ι->QL::QLAwait<>::Task{
		try{
			optional<jvalue> permissions;
			optional<jvalue> roleMembers;
			string permissionsKey = "permissionRights";
			bool permissionsPlural = true;
			// QL: role( id:16 ){ permissionRight{id allowed denied resource(target:"users",criteria:null)} }
			if( auto permissionQL = Query.ExtractTable(permissionsKey); permissionQL ){
				if( permissionsPlural = permissionQL->IsPlural(); !permissionsPlural ){
					permissionQL->JsonName = permissionsKey;//bring back array could be single right for multiple roles.
					permissionsKey = "permissionsRight";
				}
				if( auto statement = PermissionsStatement(*permissionQL); statement ){
					auto rights = co_await QL::QLAwait<jvalue>{ move(*permissionQL), move(*statement), UserPK };
					permissions = move( rights.get_array() );
				}
			}
			string roleKey = "roles";
			bool rolePlural = true;
			if( auto roleQL = Query.ExtractTable("roles"); roleQL ){
				if( auto statement = RoleStatement(*roleQL); statement ){
					rolePlural = roleQL->IsPlural();
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
			auto createRolesFromMembers = [&roles]( jvalue& roleMembers, str memberName, bool plural ){
				auto addRoleMember = [&]( jobject& member ){
					let parentRolePK = Json::AsNumber<RolePK>( member, "parentRoleId" );
					member.erase( "parentRoleId" );
					auto role = roles.try_emplace( parentRolePK, jobject{{"id", parentRolePK}} );
					auto& jmember = role.first->second;
					if( role.second || !jmember.contains(memberName) ){ //first row
						if( plural )
							jmember[memberName] = jarray{ move(member) };
						else
							jmember[DB::Names::ToSingular( memberName )] = move( member );
					}
					else if( plural )
						jmember[memberName].get_array().emplace_back( move(member) );
				};
				Json::Visit( roleMembers, addRoleMember );
			};

			if( permissions )
				createRolesFromMembers( *permissions, "permissionRights", permissionsPlural );
			if( roleMembers )
				createRolesFromMembers( *roleMembers, "roles", rolePlural );
			let& roleTable = GetTable( "roles" );
			if( !Query.FindColumn("id") )
				Query.Columns.push_back( QL::ColumnQL{"id", roleTable.GetPK()} );
			let returnArray = Query.IsPlural();
			if( auto roleStatement = QL::SelectStatement(Query); roleStatement ){
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
						role[permissionsKey] = permissionsPlural ? ( jvalue )jarray{} : ( jvalue )jobject{};
					if( roleMembers && !role.contains(roleKey) )
						role[roleKey] = rolePlural ? ( jvalue )jarray{} : ( jvalue )jobject{};
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