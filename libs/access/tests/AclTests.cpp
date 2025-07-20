#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
#include <jde/ql/QLAwait.h>
#include <jde/access/types/Resource.h>
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	α GetRolePermission( RolePK rolePK, sv resourceName, UserPK executer )ε->jobject;
	α AddRolePermission( RolePK rolePK, sv resourceName, ERights allowed, ERights denied, UserPK executer )ε->jobject;
	α AddRoleMember( RolePK parentRolePK, RolePK childRolePK, UserPK executer )ε->jobject;
	α RemoveRolePermission( RolePK rolePK, PermissionPK permissionPK, UserPK executer )ε->jobject;
	using namespace Json;
	class AclTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		α TestEnabeledPermissions( str resourceName, str target, UserPK executer )ε;
		static flat_map<string,jobject> _users;
		static flat_map<string,UserPK> _usersPKs;
		static ResourcePK _resourcePK;
	};
	flat_map<string,jobject> AclTests::_users;
	flat_map<string,UserPK> AclTests::_usersPKs;
	ResourcePK AclTests::_resourcePK;

	α SelectAcl( IdentityPK identityPK, string resourceTarget )ε->jobject{
		let ql = Ƒ( "acl( identityId:{} )< identityId permissionRight< id allowed denied resource(target:\"{}\")<deleted>> >", identityPK.Underlying(), resourceTarget );
		let acl = QL().QuerySync<jarray>( Str::Replace(Str::Replace(ql,"<","{"), ">", "}"), GetRoot() );
		return acl.empty() ? jobject{} : Json::AsObject(acl[0], "/permissionRight");
	}
	α SelectAcl( IdentityPK identityPK, RolePK rolePK )ε->jobject{
		let ql = Ƒ( "acl(identityId:{})<identityId role(id:{})<id target deleted>>", identityPK.Underlying(), rolePK );
		let acl = QL().QuerySync<jarray>( Str::Replace(Str::Replace(ql,"<","{"), ">", "}"), GetRoot() );
		return acl.empty() ? jobject{} : Json::AsObject(acl[0], "/role");
	}
	α CreateAcl( IdentityPK identityPK, ERights allowed, ERights denied, string resource, UserPK executer )ε->PermissionRightsPK{
		let resourcePK = AsNumber<ResourcePK>( SelectResource(resource, {UserPK::System}, true), "id" );
		let create = Ƒ( "createAcl( identity:{{ id:{} }}, permissionRight:{{ allowed:{}, denied:{}, resource:{{id:{}}}}} ){{permissionRight{{id}}}}", identityPK.Underlying(), underlying(allowed), underlying(denied), resourcePK );
		let createJson = QL().QuerySync( create, executer );
		return Json::AsNumber<PermissionRightsPK>( createJson, "permissionRight/id" );
	}
	α CreateAcl( IdentityPK identityPK, RolePK rolePK, UserPK executer )ε->void{
		let existing = SelectAcl( identityPK, rolePK );
		if( existing.empty() ){
			let create = Ƒ( "mutation createAcl( identity:{{ id:{} }}, role:{{id:{}}} )", identityPK.Underlying(), rolePK );
			QL().QuerySync<jvalue>( create, executer );
		}
	}

	α GetAcl( IdentityPK identityPK, string resource, ERights allowed, ERights denied )ε->jobject{
		auto entry = SelectAcl( identityPK, resource );
		if( entry.empty() ){
			CreateAcl( identityPK, allowed, denied, resource, GetRoot() );
			entry = SelectAcl( identityPK, resource );
		}
		else{
			let existingAllowed = (ERights)Json::AsNumber<uint8>( entry, "allowed" ); //ToRights( Json::AsArray(entry, "allowed") );
			let existingDenied = (ERights)Json::AsNumber<uint8>( entry, "denied" ); //ToRights( Json::AsArray(entry, "denied") );
			if( allowed!=existingAllowed || existingDenied!=denied ){
				let update = Ƒ( "mutation updatePermissionRight( id:{}, allowed:{}, denied:{} )", Json::AsNumber<PermissionPK>(entry, "id"), underlying(allowed), underlying(denied) );
				let updateJson = QL().QuerySync( update, GetRoot() );
				entry = SelectAcl( identityPK, resource );
			}
		}
		return entry;
	}
	α restoreResource( string name, UserPK executer )ε->void{
		auto resource = SelectResource( name, executer, true );
		if( !resource.at("deleted").is_null() )
			Restore( "resources", GetId(resource), executer );
	}

	α AclTests::SetUpTestCase()->void{
		array<string,10> users{ "intruder", "creator", "reader", "updater", "deleter", "purger", "admin", "subscriber", "executor", "root" };
		let resourceTarget = "groupings";
		let resource = SelectResource( resourceTarget, GetRoot(), true );
		_resourcePK = GetId( resource );
		if( resource.at("deleted").is_null() )
			Delete( "resources", GetId(resource), GetRoot() );

		auto allowed = ERights::None;
		for( let& user : users ){
			let& juser = Tests::Get( "user", user, GetRoot() );
			UserPK userPK{ GetId(juser) };
			_usersPKs.emplace( user, userPK );
			let acl = GetAcl( userPK, resourceTarget, allowed, ERights::None );
			allowed = allowed==ERights::None
				? ERights::Create
				: allowed==ERights::Execute ? ERights::All : (ERights)(underlying(allowed)<<1);
			_users.emplace( user, acl );
		}
	}

	TEST_F( AclTests, DisabledPermissions ){
		let resourceName = "groupings";
		let resource = SelectResource( resourceName, GetRoot() );
		if( resource.at("deleted").is_null() )
			Delete( "resources", GetId(resource), GetRoot() );
		let intruderPK = _usersPKs["intruder"];
		let groupId = TestCrud( "grouping", "DisabledPermission-Test-Member", intruderPK );
		TestAdd( resourceName, groupId, {_usersPKs["intruder"].Value, _usersPKs["creator"].Value, _usersPKs["reader"].Value}, intruderPK );
		TestRemove( resourceName, groupId, {_usersPKs["intruder"].Value, _usersPKs["creator"].Value}, intruderPK );
		TestPurge( resourceName, groupId, intruderPK );
	}

	α AclTests::TestEnabeledPermissions( str resourceName, str target, UserPK executer )ε{
		let groupId = TestUnauthCrud( resourceName, target, executer );
		TestUnauthAddRemove( resourceName, groupId, {_usersPKs["intruder"].Value, _usersPKs["creator"].Value, _usersPKs["reader"].Value}, executer );
		TestUnauthPurge( resourceName, groupId, executer );
		EXPECT_THROW( CreateAcl(_usersPKs["intruder"], ERights::All, ERights::None, resourceName, executer), IException );
		let rolePK = GetId( Get("role", "EnabledPermissionsTest", GetRoot()) );
		if( let existingPermission = GetRolePermission( rolePK, resourceName, GetRoot() ); !existingPermission.empty() )
			RemoveRolePermission( rolePK, GetId(existingPermission), GetRoot() );
		EXPECT_THROW( AddRolePermission(rolePK, resourceName, ERights::All, ERights::None, executer), IException );
	}

	TEST_F( AclTests, EnabledPermissions ){
		let resourceName = "groupings";
		restoreResource( resourceName, GetRoot() );
		TestEnabeledPermissions( resourceName, "EnabledPermissions-Group3", _usersPKs["intruder"] );
		Trace{ ELogTags::Test, "EnabledPermissions" };
	}

	TEST_F( AclTests, DeletedUser ){
		let resourceName = "groupings";
		restoreResource( resourceName, GetRoot() );
		auto juser = GetUser( "deletedRoot", GetRoot(), true );
		UserPK executer{ GetId( juser ) };
		GetAcl( executer, "groupings", ERights::All, ERights::None );
		if( !Json::AsTimePointOpt(juser, "deleted") )
			Delete( "users", GetId(juser), GetRoot() );
		TestEnabeledPermissions( resourceName, "AclTests-DeletedUser-Member", executer );
	}

	TEST_F( AclTests, TestHierarchy ){
		let groupResource = "groupings";
		restoreResource( groupResource, GetRoot() );
		let adminGroup = Tests::GetGroup( "HierarchyGroupAdmin", GetRoot() );
		GroupPK adminGroupPK{ GetId( adminGroup ) };

		let userGroup = Tests::GetGroup( "HierarchyGroupUsers", GetRoot() );
		let userGroupMembers = AsArray( userGroup, "groupMembers" );
		GroupPK userGroupPK{ GetId( userGroup ) };
		UserPK hierarchyUser{ GetId( GetUser("hierarchyUser", GetRoot()) ) };
		if( find_if(userGroupMembers, [=](const jvalue& member){ return GetId(Json::AsObject(member))==hierarchyUser.Value; })==userGroupMembers.end() )
			AddToGroup( userGroupPK, {hierarchyUser,adminGroupPK}, GetRoot() );
		const UserPK adminPK{ GetId( GetUser("hierarchyAdmin", GetRoot()) ) };
		let adminGroupMembers = AsArray( adminGroup, "groupMembers" );
		if( find_if(adminGroupMembers, [adminPK](const jvalue& member){ return GetId(Json::AsObject(member))==adminPK.Value; })==adminGroupMembers.end() )
			AddToGroup( adminGroupPK, {adminPK}, GetRoot() );

		let userRolePK = GetId( Get("role", "HierarchyGroupUserRole", GetRoot()) );
		AddRolePermission( userRolePK, groupResource, ERights::Read, ERights::None, GetRoot() );
		let adminRolePK = GetId( Get("role", "HierarchyGroupAdminRole", GetRoot()) );
		AddRolePermission( adminRolePK, groupResource, ERights::All & ~ERights::Read, ERights::None, GetRoot() );
		AddRoleMember( adminRolePK, userRolePK, GetRoot() );
		CreateAcl( userGroupPK, userRolePK, GetRoot() );
		CreateAcl( adminGroupPK, adminRolePK, GetRoot() );

		string testGroupTarget{ "hierarchyGroupTest" };
		auto testGroup = SelectGroup( testGroupTarget, hierarchyUser, true );
		if( !testGroup.empty() )
			PurgeGroup( {GetId(testGroup)}, adminPK );
		EXPECT_THROW( Create(groupResource, testGroupTarget, hierarchyUser), IException );
		Create( groupResource, testGroupTarget, adminPK );
		testGroup = GetGroup( testGroupTarget, hierarchyUser );
		GroupPK testGroupPK{ GetId(testGroup) };
		TestUnauthUpdateName( groupResource, testGroupPK.Value, hierarchyUser, "newName" );
		TestUnauthDeleteRestore( groupResource, testGroupPK.Value, hierarchyUser );
		vector<uint> members{ userGroupPK.Value,adminGroupPK.Value, hierarchyUser.Value, adminPK.Value };
		TestUnauthAddRemove( groupResource, testGroupPK.Value, members, hierarchyUser );
		TestUnauthPurge( groupResource, testGroupPK.Value, hierarchyUser );
		PurgeGroup( testGroupPK, adminPK );

		let testGroupPK2 = TestCrud( groupResource, testGroupTarget, adminPK );
		TestAdd( groupResource, testGroupPK2, members, adminPK );
		TestRemove( groupResource, testGroupPK2, {hierarchyUser.Value, adminPK.Value}, adminPK );
		TestPurge( groupResource, testGroupPK2, adminPK );

		EXPECT_THROW( CreateAcl(_usersPKs["intruder"], ERights::All, ERights::None, groupResource, hierarchyUser), IException );
		let rolePK = GetId( Get("role", "HierarchyPermissionsTest", GetRoot()) );
		if( auto permission = GetRolePermission(rolePK, groupResource, GetRoot()); !permission.empty() )
			RemoveRolePermission( rolePK, GetId(permission), GetRoot() );
		EXPECT_THROW( AddRolePermission(rolePK, groupResource, ERights::All, ERights::None, hierarchyUser), IException );
	}
	TEST_F( AclTests, TestDeny ){
		let resourceName = "groupings";
		restoreResource( resourceName, GetRoot() );

		let deniedGroup = Tests::GetGroup( "DeniedGroup", GetRoot() );
		let deniedGroupPK = GetId( deniedGroup );
		let allowedGroup = Tests::GetGroup( "allowedGroup", GetRoot() );
		let allowedGroupMembers = AsArray( allowedGroup, "groupMembers" );
		let allowedGroupPK = GetId( allowedGroup );
		UserPK executer{ GetId(GetUser("deniedUser", GetRoot())) };
		if( find_if(allowedGroupMembers, [executer](const jvalue& member){ return GetId(Json::AsObject(member))==executer.Value; })==allowedGroupMembers.end() )
			AddToGroup( {allowedGroupPK}, {executer}, GetRoot() );
		let deniedGroupMembers = AsArray( deniedGroup, "groupMembers" );
		if( find_if(deniedGroupMembers, [executer](const jvalue& member){ return GetId(Json::AsObject(member))==executer.Value; })==deniedGroupMembers.end() )
			AddToGroup( {deniedGroupPK}, {executer}, GetRoot() );

		let deniedRolePK = GetId( Get("role", "DeniedRole", GetRoot()) );
		AddRolePermission( deniedRolePK, resourceName, ERights::None, ERights::All, GetRoot() );
		let allowedRolePK = GetId( Get("role", "AllowedRole", GetRoot()) );
		AddRolePermission( allowedRolePK, resourceName, ERights::All, ERights::None, GetRoot() );
		CreateAcl( GroupPK{deniedGroupPK}, deniedRolePK, GetRoot() );
		CreateAcl( GroupPK{allowedGroupPK}, allowedRolePK, GetRoot() );
		TestEnabeledPermissions( resourceName, "AclTests-TestDeny-Group", executer );
	}
	//remove user from group/role.
	//purge acl
}