#include "AccessListener.h"
#include <jde/ql/ql.h>
#include <jde/ql/IQL.h>
#include <jde/ql/SubscriptionAwait.h>
#include <jde/access/Authorize.h>

#define let const auto

namespace Jde::Access{
	sp<AccessListener> _listener;
	α AccessListener::Instance()ι->sp<AccessListener>{ return _listener; }
	α AccessListener::SetInstance( sp<QL::IQL> qlServer )ι->void{
		ASSERT( !AccessListener::Instance() );
		_listener = ms<AccessListener>( qlServer );
		Process::AddShutdown(  _listener );
	}

	α AccessListener::Shutdown( bool terminate )ι->void{
		if( terminate )
			return;
		try{
			BlockVoidAwait<QL::UnsubscribeAwait>( _qlServer->Unsubscribe( move(Ids) ) );
		}
		catch( IException& e ){
		}
		_listener = nullptr;
	}
	α AccessListener::OnChange( const jvalue& j, QL::SubscriptionClientId clientId )ε->void{
		let& root = Json::AsObject(j);
		let nameValue = root.begin();
		if( nameValue==root.end() )
			return;
		let& object = Json::AsObject( nameValue->value() );
		let event = (ESubscription)clientId;
		using enum ESubscription;
		if( !empty(event & Acl) ){
			AclChanged( event & ~Acl, object );
			return;
		}

		let pk = Json::AsNumber<uint>( object, "id" );
		if( !empty(event & User) )
			UserChanged( {pk}, event & ~User, object );
		else if( !empty(event & Group) )
			GroupChanged( {pk}, event & ~Group, object );
		else if( !empty(event & Role) )
			RoleChanged( pk, event & ~Role, object );
		else if( !empty(event & Resources) )
			ResourceChanged( pk, event & ~Resources, object );
		else if( !empty(event & Permission) )
			PermissionUpdated( pk, object );
	}
#pragma GCC diagnostic ignored "-Wswitch"
	α AccessListener::UserChanged( UserPK userPK, ESubscription event, const jobject& o )ι->void{
		using enum ESubscription;
		switch( event ){
			case Created: Authorizer().CreateUser( userPK ); break;
			case Deleted: Authorizer().DeleteUser( userPK ); break;
			case Restored: Authorizer().RestoreUser( userPK ); break;
			case Purged: Authorizer().PurgeUser( userPK ); break;
		}
	}
	α AccessListener::GroupChanged( GroupPK groupPK, ESubscription event, const jobject& o )ε->void{
		using enum ESubscription;
		switch( event ){
			case Added:
			case Removed:{
				flat_set<IdentityPK::Type> members;
				Json::Visit( Json::AsValue(o, "memberId"), [&](const jvalue& v){ members.insert( Json::AsNumber<IdentityPK::Type>(v) );} );
				if( event==Added )
					Authorizer().AddToGroup( groupPK, members );
				else
					Authorizer().RemoveFromGroup( groupPK, members );
			}break;
			case Deleted:
				Authorizer().DeleteGroup( groupPK );
				break;
			case Restored:
				Authorizer().RestoreGroup( groupPK );
				break;
			case Purged:
				Authorizer().PurgeGroup( groupPK );
				break;
		}
	}
	α AccessListener::RoleChanged( RolePK rolePK, ESubscription event, const jobject& o )ε->void{
		using enum ESubscription;
		switch( event ){
			case Deleted: Authorizer().DeleteRestoreRole( rolePK, true ); break;
			case Restored: Authorizer().DeleteRestoreRole( rolePK, false ); break;
			case Purged: Authorizer().PurgeRole( rolePK ); break;
			case Added:
			case Removed:{
				if( auto rights = Json::FindObject(o, "permissionRight"); rights ){
					if( event==Added ){
						Access::Permission permission{ *rights };
						Authorizer().AddRolePermission( rolePK, permission.PK, (ERights)permission.Allowed, (ERights)permission.Denied, Json::AsSVPath(*rights, "resource/target") );
					}
					else{
						flat_set<PermissionRightPK> members;
						Json::Visit( Json::AsValue(o, "permissionRight/id"), [&](const jvalue& v){ members.insert( Json::AsNumber<PermissionRightPK>(v) );} );
						Authorizer().RemoveRoleChildren( rolePK, members );
					}
				}
				else if( auto child = Json::FindObject(o, "role"); child ){
					if( event==Added )
						Authorizer().AddRoleChild( rolePK, Json::AsNumber<RolePK>(*child, "id") );
					else{
						flat_set<PermissionRightPK> members;
						Json::Visit( Json::AsValue(o, "id"), [&](const jvalue& v){ members.insert( Json::AsNumber<RolePK>(v) );} );
						Authorizer().RemoveRoleChildren( rolePK, members );
					}
				}
			}break;
		}
	}
	α AccessListener::ResourceChanged( ResourcePK resourcePK, ESubscription event, const jobject& o )ε->void{
		using enum ESubscription;
		switch( event ){
			case Created: Authorizer().CreateResource( {resourcePK, o} ); break;
			case Deleted:
			case Restored:
				Authorizer().UpdateResourceDeleted( Json::FindDefaultSV(o, "schema"), o, event==Restored );
				break;
		}
	}
	α AccessListener::PermissionUpdated( PermissionRightsPK pk, const jobject& o )ε->void{
		Access::Permission permission{ o };
		let allowed = Json::FindNumber<uint8>( o, "allowed" );
		let denied = Json::FindNumber<uint8>( o, "denied" );
		Authorizer().UpdatePermission( pk, allowed ? optional<ERights>((ERights)*allowed) : nullopt, denied ? optional<ERights>((ERights)*denied) : nullopt );
	}
	α AccessListener::AclChanged( ESubscription event, const jobject& o )ι->void{
		using enum ESubscription;
		let identityPK = Json::AsNumber<IdentityPK::Type>( o, "identity/id" );
		switch( event ){
			case Created:{
				if( auto v = o.if_contains("permissionRight"); v ){ //identity{id:y}, permission:{ allowed:x, denied:x, resource:{id:x} }
					let& permission = Json::AsObject(*v);
					Authorizer().AddAcl(
						identityPK,
						Json::AsNumber<PermissionRightsPK>( permission, "id" ),
						(ERights)Json::FindNumber<uint8>( permission, "allowed" ).value_or(0),
						(ERights)Json::FindNumber<uint8>( permission, "denied" ).value_or(0),
						Json::AsNumber<ResourcePK>( permission, "resource/id" ) );
				}
				else if( auto role = o.if_contains("role"); role ) //identity{id:y}, role:{ id:x }
					Authorizer().AddAcl( Json::AsNumber<IdentityPK::Type>( o, "identity/id" ), QL::AsId<RolePK>(*role) );
			}break;
			case Purged:{
				optional<PermissionRole> permissionPK;
				if( auto p = o.if_contains("permission"); p ) //identity{id:y}, permission:{ allowed:x, denied:x, resource:{id:x} }
					permissionPK = PermissionRole{ std::in_place_index<0>, QL::AsId<PermissionRightsPK>(*p) };
				else if( auto role = o.if_contains("role"); role ) //identity{id:y}, role:{ id:x }
					permissionPK = PermissionRole{ std::in_place_index<1>, QL::AsId<RolePK>(*role) };
				if( permissionPK )
					Authorizer().RemoveAcl( identityPK, *permissionPK );
			}break;
		}
	}
}