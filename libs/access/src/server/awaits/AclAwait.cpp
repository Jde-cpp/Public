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

#define let const auto
namespace Jde::Access::Server{
	α AclQLAwait::Table()ε->const DB::View&{ return GetTable("acl"); }

	α AclQLAwait::Suspend()ι->void{
		if( _mutation.Type==QL::EMutationQL::Purge )
			PurgeAcl();
		else if( _mutation.Type==QL::EMutationQL::Create )
			InsertAcl();
	}
	//{ mutation purgeAcl( identity:{id:7}, permissionRight:{id:42} ) } - or role:{id:42}.  A direct grant and a role assignment are
	//both access_acl rows in one pk space, so the pk is resolved first and the gate comes from what it *is*
	//(access_permissions.is_role), not from which key the client chose:  keyed on the spelling, a roles admin could revoke any
	//identity's direct grant on any resource by sending its pk as role:{id} (access-review3 #11).  Either spelling is accepted.
	α AclQLAwait::PurgeAcl()ι->DB::ScalerAwaitOpt<uint>::Task{
		try{
			let args = _mutation.ExtrapolateVariables();
			let identityPK = Json::AsNumber<IdentityPK::Type>( args, "identity/id" );
			auto permissionPK = Json::FindNumberPath<PermissionPK>( args, "permissionRight/id" );
			if( !permissionPK )
				permissionPK = Json::FindNumberPath<PermissionPK>( args, "role/id" );
			THROW_IF( !permissionPK, "Could not find permissionRight or role id in '{}'", serialize(args) );
			let isRole = co_await DS().ScalerOpt<uint>( DB::Sql{Ƒ("select is_role from {} where permission_id=?", GetTable("permissions").DBName), vector<DB::Value>{{*permissionPK}}} );
			THROW_IF( !isRole, "[{}]Permission not found.", *permissionPK );
			if( *isRole )
				Authorizer().TestAdmin( "roles", _executer, _sl );
			else
				Authorizer().TestAdminPermission( *permissionPK, _executer, _sl );
			//the listener - and any subscriber - branches on the key, so the notification has to carry the one the pk is.
			const sv key = *isRole ? "role" : "permissionRight", other = *isRole ? "permissionRight" : "role";
			_mutation.Args.erase( other );
			_mutation.Args[key] = jobject{ {"id", *permissionPK} };
			PurgeAcl( identityPK, *permissionPK, *isRole!=0 );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::PurgeAcl( IdentityPK::Type identityPK, PermissionPK permissionPK, bool isRole )ι->DB::ExecuteAwait::Task{
		try{
			let ds = Table().Schema->DS();
			let aclCount = co_await ds->Execute(
				DB::Sql{ Ƒ("delete from {} where identity_id=? and permission_id=?", Table().DBName), vector<DB::Value>{{identityPK}, {permissionPK}} }, _sl );
			if( aclCount && !isRole ){ //a direct grant's rights row goes with its last acl link;  a role has no rights row, and the acl row was the whole assignment.
				co_await ds->Execute(
					DB::Sql{ Ƒ("delete from {} where permission_id=? and not exists( select 1 from {} where permission_id=? )", GetTable("permission_rights").DBName, Table().DBName), vector<DB::Value>{{permissionPK}, {permissionPK}} }, _sl );
			}
			jobject y;
			y["rowCount"] = aclCount;
			//y["complete"] = true;
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( move(y) );
		}
		catch( runtime_error& e ){
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
			Authorizer().TestAdmin( "roles", _executer, _sl );
			DB::InsertClause insert{ Table().InsertProcName()+"_role" };
			let args = _mutation.ExtrapolateVariables();
			let identityPK = Json::AsNumber<IdentityPK::Type>( args, "identity/id" );
			insert.Add( identityPK );
			let rolePK = Json::AsNumber<ResourcePK>( args, "role/id" );
			insert.Add( rolePK );
			y["rowCount"] = co_await DS().Execute( insert.Move() );
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( jvalue{y} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLAwait::InsertPermission( const jobject& permission )ι->TAwait<optional<ResourcePK>>::Task{
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
		catch( runtime_error& e ){
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
			QL::Subscriptions::OnMutation( _mutation, y );
			Resume( y );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}

	α AclQLSelectAwait::Suspend()ι->void{
		try{
			GetTable( "acl" ).Authorize( Access::ERights::Read, _executer, _sl );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
			return;
		}
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
	α AclQLSelectAwait::GetStatement( const QL::TableQL& childTable, sp<DB::Column> joinColumn )ε->DB::Statement{
		let& table = GetTable( "acl" );
		auto statement = QL::SelectStatement( childTable );
		auto aclStatement = QL::SelectStatement( Query );
		statement.Select += move( aclStatement.Select );
		statement.Where += aclStatement.Where;
		statement.From += { joinColumn, table.GetColumnPtr("permission_id"), true };
		if( auto identities = Query.FindTable("identities"); identities ){
			if( !(identities->Columns.size()==1 && identities->Columns.front().JsonName=="id") )
				statement.From += { table.GetColumnPtr("identity_id"), GetTable("identities").GetColumnPtr("identity_id"), true };
		}
		return statement;
	}
	α AclQLSelectAwait::LoadIdentities( const QL::TableQL& identitiesQL )ι->DB::SelectAwait::Task{
		jarray identities;
		try{
			auto statement = QL::SelectStatement( identitiesQL );
			auto aclStatement = QL::SelectStatement( Query );
			aclStatement.Select += move( statement.Select );
			aclStatement.From = DB::Join{ GetTable("acl").GetColumnPtr("identity_id"), GetTable("identities").GetColumnPtr("identity_id"), true };
			auto rows = co_await DS().SelectAsync( aclStatement.Move() );
			let& columns = aclStatement.Select.Columns;
			for( auto& row : rows ){
				jobject jrow;
				for( uint i=0; i<row.Size() && i<columns.size(); ++i )
					identitiesQL.SetResult( jrow, get<DB::AliasCol>(columns[i]).Column, move(row[i]) );
				identities.emplace_back( move(jrow) );
			}
			jobject o{ {"identities", identities} };
			if( Query.IsPlural() )
				Resume( jarray{o} );
			else
				Resume( o );
		}
		catch( runtime_error& e ){
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
			auto statement = GetStatement( roleQL, GetTable("roles").GetColumnPtr("role_id") );
			auto rows = co_await DS().SelectAsync( statement.Move() );
			let& columns = statement.Select.Columns;
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
			Resume( move(y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLSelectAwait::LoadPermissions( const QL::TableQL& permissionsQL )ι->DB::SelectAwait::Task{
		jarray y;
		try{
			auto statement = GetStatement( permissionsQL, GetTable("permissions").GetColumnPtr("permission_id") );
			auto rows = co_await DS().SelectAsync( statement.Move() );
			let& columns = statement.Select.Columns;
			for( auto& row : rows ){
				jobject jrow;
				for( uint i=0; i<row.Size() && i<columns.size(); ++i )
					Query.SetResult( jrow, get<DB::AliasCol>(columns[i]).Column, move(row[i]) );
				y.emplace_back( move(jrow) );
			}
			Resume( move(y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α AclQLSelectAwait::LoadPermissionRights( const QL::TableQL& permissionRights )ι->DB::SelectAwait::Task{
		jarray y;
		try{
			auto statement = GetStatement( permissionRights, GetTable("permission_rights").GetColumnPtr("permission_id") );
			auto rows = co_await DS().SelectAsync( statement.Move() );
			let& columns = statement.Select.Columns;
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
			Resume( move(y) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}