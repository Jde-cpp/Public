#include <jde/access/server/awaits/AclAwait.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/Table.h>
#include <jde/db/names.h>
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/LocalSubscriptions.h>
#include <jde/ql/QLAwait.h>
#include <jde/access/types/Resource.h>
#include <jde/access/Authorize.h>
#include "../serverInternal.h"
#include "../../accessInternal.h"
#pragma GCC diagnostic ignored "-Wdangling-reference"

#define let const auto
namespace Jde::Access::Server{
	α AclQLAwait::Table()ε->const DB::View&{ return GetTable("acl"); }

	α AclQLAwait::Suspend()ι->void{
		if( _mutation.Type==QL::EMutationQL::Purge )
			PurgeAcl();
		else if( _mutation.Type==QL::EMutationQL::Create )
			InsertAcl();
	}
	α AclQLAwait::PurgeAcl()ι->QL::QLAwait<jobject>::Task{
		try{
			let identityPK = Json::AsNumber<IdentityPK::Type>( _mutation.Args, "identity/id" );
			auto permissionPK = Json::FindNumberPath<PermissionPK>( _mutation.Args, "permissionRight/id" );
			if( !permissionPK ){
				permissionPK = Json::FindNumberPath<PermissionPK>( _mutation.Args, "role/id" );
				Authorizer().TestAdmin( "roles", _executer, _sl );
			}
			else{
				let resourcePK = co_await QL::QLAwait<jobject>( Ƒ("permissionRight(id:{}){{resource{{id deleted}}}}", *permissionPK), {}, {UserPK::System}, {GetSchemaPtr()} );
				Authorizer().TestAdmin( Json::AsNumber<ResourcePK>(resourcePK, "resource/id"), _executer, _sl );
			}
			THROW_IF( !permissionPK, "Could not find permissionRight or role id in '{}'", serialize(_mutation.Args) );
			PurgeAcl( identityPK, *permissionPK );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::PurgeAcl( IdentityPK::Type identityPK, PermissionPK permissionPK )ι->DB::ExecuteAwait::Task{
		try{
			let ds = Table().Schema->DS();
			let aclCount = co_await ds->Execute(
				DB::Sql{ Ƒ("delete from {} where identity_id=? and permission_id=?", Table().DBName), vector<DB::Value>{{identityPK}, {permissionPK}} }, _sl );
			co_await ds->Execute(
				DB::Sql{ Ƒ("delete from {} where permission_id=?", GetTable("permission_rights").DBName), vector<DB::Value>{{permissionPK}} }, _sl );
			jobject y;
			y["rowCount"] = aclCount;
			//y["complete"] = true;
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( move(y) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::InsertAcl()ι->void{
		auto input = _mutation.ExtrapolateVariables();
		if( auto p = input.find("permissionRight"); p!=input.end() && p->value().is_object() ) //identity{ id:x }, permission:{ allowed:x, denied:x, resource:{id:x} }
			InsertPermission( p->value().get_object() );
		else if( auto r = input.find("role"); r!=input.end() ) //identity{ id:x }, role:{ id:x }
			InsertRole();
		else
			ResumeExp( Exception{"Invalid ACL mutation"} );
	}
	α AclQLAwait::InsertRole()ι->DB::ExecuteAwait::Task{
		jobject y;
		try{
			DB::InsertClause insert{ Table().InsertProcName()+"_role" };
			let args = _mutation.ExtrapolateVariables();
			let identityPK = Json::AsNumber<IdentityPK::Type>( args, "identity/id" );
			insert.Add( identityPK );
			let rolePK = Json::AsNumber<ResourcePK>( args, "role/id" );
			insert.Add( rolePK );
			y["rowCount"] = co_await DS().Execute( insert.Move() );
			//y["complete"] = true;
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( jvalue{y} );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::InsertPermission( const jobject& permission )ι->DB::ScalerAwait<optional<ResourcePK>>::Task{
		let allowed = ( ERights )Json::FindNumber<uint8>( permission, "allowed" ).value_or( 0 );
		let denied = ( ERights )Json::FindNumber<uint8>( permission, "denied" ).value_or( 0 );
		try{
			auto& resource = permission.at("resource").as_object();
			auto key = Json::AsKey( resource );
			if( !key.IsPK() ){
				auto criteria = Json::FindString( resource, "criteria" );
				auto dbCriteria = criteria ? DB::Value{move(*criteria)} : DB::Value{nullptr};
				auto resPK = co_await DS().ScalerOpt<ResourcePK>({
					Ƒ( "select resource_id from {} where schema_name=? and target=? and coalesce(criteria, '')=coalesce(?, '')", GetTable("resources").DBName ),
					{ DB::Value{Json::AsString(resource, "schemaName")}, DB::Value::FromKey(key.NK()), dbCriteria }
				});
				if( resPK ){
					key = *resPK;
					_mutation.Args.at("permissionRight").at("resource").as_object()["id"] = key.PK(); //for subscriptions
				}else
					THROW( "Resource not found for target '{}' schema '{}'", key.NK(), Json::AsString(resource, "schemaName") );//TODO implement TestAdmin for this
			}
			InsertPermission( allowed, denied, key.PK() );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::InsertPermission( ERights allowed, ERights denied, ResourcePK resourcePK )ι->DB::ScalerAwait<PermissionPK>::Task{
		try{
			Authorizer().TestAdmin( resourcePK, _executer, _sl );
			DB::InsertClause insert{ Table().UpsertProcName()+"_permission" };
			let identityPK = _mutation.AsPathNumber<IdentityPK::Type>( "identity/id" );
			insert.Add( identityPK );

			insert.Add( underlying(allowed) );
			insert.Add( underlying(denied) );
			insert.Add( resourcePK );
			let permissionPK = co_await DS().InsertSeq<PermissionPK>( move(insert) );
			jobject y;
			y["permissionRight"].emplace_object()["id"] = permissionPK;
			//y["complete"]=true;
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( y );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α AclQLSelectAwait::Suspend()ι->void{
		if( auto rights = Query.FindTable("permissionRights"); rights )
			LoadPermissionRights( *rights );
		else if( auto roles = Query.FindTable("roles"); roles )
			LoadRoles( *roles );
		else if( auto roles = Query.FindTable("permissions"); roles )
			LoadPermissions( *roles );
		else if( auto identities = Query.FindTable("identities"); identities )
			LoadIdentities( *identities );
		else
			ResumeExp( Exception{"query not implemented"} );
	}
	α AclQLSelectAwait::GetStatement( const QL::TableQL& childTable, sp<DB::Column> joinColumn )ε->optional<DB::Statement>{
		let& table = GetTable( "acl" );
		auto statement = QL::SelectStatement( childTable );
		if( auto aclStatement = !statement ? optional<DB::Statement>{} : QL::SelectStatement(Query); aclStatement ){
			statement->Select += move( aclStatement->Select );
			statement->Where += aclStatement->Where;
			statement->From += { joinColumn, table.GetColumnPtr("permission_id"), true };
			if( auto identities = Query.FindTable("identities"); identities ){
				if( !(identities->Columns.size()==1 && identities->Columns.front().JsonName=="id") )
					statement->From += { table.GetColumnPtr("identity_id"), GetTable("identities").GetColumnPtr("identity_id"), true };
			}
		}
		return statement;
	}
	α AclQLSelectAwait::LoadIdentities( const QL::TableQL& identitiesQL )ι->DB::SelectAwait::Task{
		jarray identities;
		try{
			auto statement = QL::SelectStatement( identitiesQL );
			if( auto aclStatement = !statement ? optional<DB::Statement>{} : QL::SelectStatement(Query); aclStatement ){
				aclStatement->Select += move( statement->Select );
				aclStatement->From = DB::Join{ GetTable("acl").GetColumnPtr("identity_id"), GetTable("identities").GetColumnPtr("identity_id"), true };
				auto rows = co_await DS().SelectAsync( aclStatement->Move() );
				let& columns = aclStatement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					for( uint i=0; i<row.Size() && i<columns.size(); ++i )
						identitiesQL.SetResult( jrow, get<DB::AliasCol>(columns[i]).Column, move(row[i]) );
					identities.emplace_back( move(jrow) );
				}
			}
			jobject o{ {"identities", identities} };
			if( Query.IsPlural() )
				Resume( jarray{o} );
			else
				Resume( o );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	Ω addIdentityColumn( jobject& jrow, jobject*& identity, str key, DB::Value&& value )ι->void{
		if( !identity )
			identity = &jrow["identity"].emplace_object();
		( *identity )[key=="identityId" ? "id" : key] = value.Move();
	}
	α AclQLSelectAwait::LoadRoles( const QL::TableQL& roleQL )ι->DB::SelectAwait::Task{
		jarray y;
		try{
			if( auto statement = GetStatement(roleQL, GetTable("roles").GetColumnPtr("role_id")); statement ){
				auto rows = co_await DS().SelectAsync( statement->Move() );
				let& columns = statement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					jobject* role{};
					jobject* identity{};
					for( uint i=0; i<row.Size() && i<columns.size(); ++i ){
						let& column = get<DB::AliasCol>( columns[i] ).Column;
						auto& value = row[i];
						let key = DB::Names::ToJson( column->Name );
						if( column->Table->Name=="roles" ){
							if( !role )
								role = &jrow["role"].emplace_object();
							( *role )[key=="roleId" ? "id" : key] = value.Move();
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
	α AclQLSelectAwait::LoadPermissions( const QL::TableQL& permissionsQL )ι->DB::SelectAwait::Task{
		jarray y;
		try{
			if( auto statement = GetStatement(permissionsQL, GetTable("permissions").GetColumnPtr("permission_id")); statement ){
				auto rows = co_await DS().SelectAsync( statement->Move() );
				let& columns = statement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					for( uint i=0; i<row.Size() && i<columns.size(); ++i )
						Query.SetResult( jrow, get<DB::AliasCol>(columns[i]).Column, move(row[i]) );
					y.emplace_back( move(jrow) );
				}
			}
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLSelectAwait::LoadPermissionRights( const QL::TableQL& permissionRights )ι->DB::SelectAwait::Task{
		jarray y;
		try{
			if( auto statement = GetStatement(permissionRights, GetTable("permission_rights").GetColumnPtr("permission_id")); statement ){
				auto rows = co_await DS().SelectAsync( statement->Move() );
				let& columns = statement->Select.Columns;
				for( auto& row : rows ){
					jobject jrow;
					jobject* right{};
					jobject* resource{};
					jobject* identity{};
					for( uint i=0; i<row.Size() && i<columns.size(); ++i ){
						let& column = get<DB::AliasCol>( columns[i] ).Column;
						auto& value = row[i];
						let key = DB::Names::ToJson( column->Name );
						if( column->Table->Name=="permission_rights" || column->Table->Name=="resources" ){
							if( !right )
								right = &jrow["permissionRight"].emplace_object();
							if( column->Table->Name=="permission_rights" )
								( *right )[key=="permissionId" ? "id" : key] = value.Move();
							else{
								if( !resource )
									resource = &( *right )["resource"].emplace_object();
								( *resource )[key=="resourceId" ? "id" : key] = value.Move();
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
}