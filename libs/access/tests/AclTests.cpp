#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
#include <jde/ql/ql.h>
#include "../src/types/Resource.h"
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	α AddRolePermission( RolePK rolePK, sv resourceName, ERights allowed, ERights denied, UserPK userPK )ε->jobject;
	α AddRoleMember( RolePK parentRolePK, RolePK childRolePK, UserPK userPK )ε->jobject;
	using namespace Json;
	class AclTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;

		α TestEnabeledPermissions( UserPK userPK )ε;
		static flat_map<string,jobject> _users;
		static flat_map<string,uint> _usersPKs;
		static ResourcePK _resourcePK;
	};
	flat_map<string,jobject> AclTests::_users;
	flat_map<string,uint> AclTests::_usersPKs;
	ResourcePK AclTests::_resourcePK;

	α SelectAcl( IdentityPK identityPK, string resourceTarget )ε->jobject{
		let ql = Ƒ( "query< acl( identityId:{} )< identityId permissionRight< id allowed denied resource(target:\"{}\")<deleted>> > >", identityPK, resourceTarget );
		let qlResult = QL::Query( Str::Replace(Str::Replace(ql,"<","{"), ">", "}"), GetRoot() );
		return Json::FindDefaultObjectPath( qlResult, "acl/permissionRight" );
	}
	α SelectAcl( IdentityPK identityPK, RolePK rolePK )ε->jobject{
		let ql = Ƒ( "query< acl(identityId:{})<identityId role(id:{})<id target deleted>> >", identityPK, rolePK );
		let qlResult = QL::Query( Str::Replace(Str::Replace(ql,"<","{"), ">", "}"), GetRoot() );
		return Json::FindDefaultObjectPath( qlResult, "acl/role" );
	}
	α CreateAcl( IdentityPK identityPK, ERights allowed, ERights denied, string resource, UserPK userPK )ε->jobject{
		let resourcePK = AsNumber<ResourcePK>( SelectResource(resource, true), "id" );
		let create = Ƒ( "mutation createAcl( input:{{ identityId:{}, permission:{{ allowed:{}, denied:{}, resource:{{id:{}}}}} }} )", identityPK, underlying(allowed), underlying(denied), resourcePK );
		let createJson = QL::Query( create, userPK );
		return Json::FindDefaultObject( createJson, "permissionRight" );
	}
	α CreateAcl( IdentityPK identityPK, RolePK rolePK, UserPK userPK )ε->void{
		let existing = SelectAcl( identityPK, rolePK );
		if( existing.empty() ){
			let create = Ƒ( "mutation createAcl( input:{{ identityId:{}, role:{{id:{}}} }} )", identityPK, rolePK );
			QL::Query( create, userPK );
		}
	}

	α GetAcl( IdentityPK identityPK, string resource, ERights allowed, ERights denied )ε->jobject{
		auto entry = SelectAcl( identityPK, resource );
		if( entry.empty() ){
			let createJson = CreateAcl( identityPK, allowed, denied, resource, GetRoot() );
			entry = SelectAcl( identityPK, resource );
		}
		else{
			let existingAllowed = (ERights)Json::AsNumber<uint8>( entry, "allowed" ); //ToRights( Json::AsArray(entry, "allowed") );
			let existingDenied = (ERights)Json::AsNumber<uint8>( entry, "denied" ); //ToRights( Json::AsArray(entry, "denied") );
			if( allowed!=existingAllowed || existingDenied!=denied ){
				let update = Ƒ( "mutation updatePermissionRight( id:{}, input:{{ allowed:{}, denied:{} }} )", Json::AsNumber<PermissionPK>(entry, "id"), underlying(allowed), underlying(denied) );
				let updateJson = QL::Query( update, 0 );
				entry = SelectAcl( identityPK, resource );
			}
		}
		return entry;
	}
	α restoreResource( string name, UserPK userPK )ε->void{
		auto resource = SelectResource( name, userPK, true );
		if( !resource.at("deleted").is_null() )
			Restore( "resources", GetId(resource), userPK );
	}

	α AclTests::SetUpTestCase()->void{
		array<string,10> users{ "intruder", "creator", "reader", "updater", "deleter", "purger", "admin", "subscriber", "executor", "root" };
		let resourceTarget = "identityGroups";
		let resource = SelectResource( resourceTarget, true );
		_resourcePK = AsNumber<UserPK>( resource, "id" );
		if( resource.at("deleted").is_null() )
			Delete( "resources", GetId(resource), GetRoot() );

		auto allowed = ERights::None;
		for( let& user : users ){
			let& juser = Tests::Get( "user", user, GetRoot() );
			let userPK = GetId( juser );
			_usersPKs.emplace( user, userPK );
			let acl = GetAcl( userPK, resourceTarget, allowed, ERights::None );
			allowed = allowed==ERights::None
				? ERights::Create
				: allowed==ERights::Execute ? ERights::All : (ERights)(underlying(allowed)<<1);
			_users.emplace( user, acl );
		}
	}

	TEST_F( AclTests, DisabledPermissions ){
		let resource = SelectResource( "identityGroups", GetRoot() );
		if( resource.at("deleted").is_null() )
			Delete( "resources", GetId(resource), GetRoot() );
		let intruderPK = _usersPKs["intruder"];
		let groupId = TestCrud( "identityGroup", "DisabledPermission-Test-Group", intruderPK );
		TestAdd( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"], _usersPKs["reader"]}, intruderPK );
		TestRemove( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"]}, intruderPK );
		TestPurge( "identityGroups", groupId, intruderPK );
	}

	α AclTests::TestEnabeledPermissions( UserPK userPK )ε{
		let resourceName = "identityGroups";
		restoreResource( resourceName, GetRoot() );
		let groupId = TestUnauthCrud( "identityGroup", "DeniedPermission-Test", userPK );
		TestUnauthAddRemove( resourceName, groupId, {_usersPKs["intruder"], _usersPKs["creator"], _usersPKs["reader"]}, userPK );
		TestUnauthPurge( resourceName, groupId, userPK );
		EXPECT_THROW( CreateAcl(_usersPKs["intruder"], ERights::All, ERights::None, resourceName, userPK), IException );
		let rolePK = GetId( Get("role", "EnabledPermissionsTest", GetRoot()) );
		EXPECT_THROW( AddRolePermission(rolePK, resourceName, ERights::All, ERights::None, userPK), IException );
	}

	TEST_F( AclTests, EnabledPermissions ){
		TestEnabeledPermissions( _usersPKs["intruder"] );
	}

	TEST_F( AclTests, DeletedUser ){
		let resourceName = "identityGroups";
		restoreResource( resourceName, GetRoot() );
		auto juser = GetUser( "deletedRoot", GetRoot(), true );
		UserPK userPK = GetId( juser );
		GetAcl( userPK, "identityGroups", ERights::All, ERights::None );
		if( !Json::AsTimePointOpt(juser, "deleted") )
			Delete( "users", GetId(juser), GetRoot() );
		TestEnabeledPermissions( userPK );
	}

	TEST_F( AclTests, TestHierarchy ){
		let resourceName = "identityGroups";
		restoreResource( resourceName, GetRoot() );
		let adminGroup = Tests::GetGroup( "HierarchyGroupAdmin", GetRoot() );
		let adminGroupPK = GetId( adminGroup );

		let userGroup = Tests::GetGroup( "HierarchyGroupUsers", GetRoot() );
		let userGroupMembers = AsArray( userGroup, "members" );
		let userGroupPK = GetId( userGroup );
		let userPK = GetId( GetUser("hierarchyUser", GetRoot()) );
		if( find_if(userGroupMembers, [userPK](const jvalue& member){ return GetId(Json::AsObject(member))==userPK; })==userGroupMembers.end() )
			AddToGroup( userGroupPK, {userPK,adminGroupPK}, GetRoot() );
		let adminPK = GetId( GetUser("hierarchyAdmin", GetRoot()) );
		let adminGroupMembers = AsArray( adminGroup, "members" );
		if( find_if(adminGroupMembers, [adminPK](const jvalue& member){ return GetId(Json::AsObject(member))==adminPK; })==adminGroupMembers.end() )
			AddToGroup( adminGroupPK, {adminPK}, GetRoot() );

		let userRolePK = GetId( Get("role", "HierarchyGroupUserRole", GetRoot()) );
		AddRolePermission( userRolePK, resourceName, ERights::Read, ERights::None, GetRoot() );
		let adminRolePK = GetId( Get("role", "HierarchyGroupAdminRole", GetRoot()) );
		AddRolePermission( adminRolePK, resourceName, ERights::All & ~ERights::Read, ERights::None, GetRoot() );
		AddRoleMember( adminRolePK, userRolePK, GetRoot() );
		CreateAcl( userGroupPK, userRolePK, GetRoot() );
		CreateAcl( adminGroupPK, adminRolePK, GetRoot() );

		string testGroupTarget{ "hierarchyGroupTest" };
		auto testGroup = SelectGroup( testGroupTarget, userPK, true );
		if( !testGroup.empty() )
			PurgeGroup( GetId(testGroup), adminPK );
		EXPECT_THROW( Create(resourceName, testGroupTarget, userPK), IException );
		Create( resourceName, testGroupTarget, adminPK );
		testGroup = GetGroup( testGroupTarget, userPK );
		let testGroupPK = GetId( testGroup );
		TestUnauthUpdateName( resourceName, testGroupPK, userPK, "newName" );
		TestUnauthDeleteRestore( resourceName, testGroupPK, userPK );
		vector<IdentityPK> members{ userGroupPK,adminGroupPK, userPK, adminPK };
		TestUnauthAddRemove( resourceName, testGroupPK, members, userPK );
		TestUnauthPurge( resourceName, testGroupPK, userPK );
		PurgeGroup( testGroupPK, adminPK );

		let testGroupPK2 = TestCrud( resourceName, testGroupTarget, adminPK );
		TestAdd( resourceName, testGroupPK2, members, adminPK );
		TestRemove( resourceName, testGroupPK2, {userPK, adminPK}, adminPK );
		TestPurge( resourceName, testGroupPK2, adminPK );

		EXPECT_THROW( CreateAcl(_usersPKs["intruder"], ERights::All, ERights::None, resourceName, userPK), IException );
		let rolePK = GetId( Get("role", "HierarchyPermissionsTest", GetRoot()) );
		EXPECT_THROW( AddRolePermission(rolePK, resourceName, ERights::All, ERights::None, userPK), IException );
		CreateAcl( _usersPKs["intruder"], ERights::All, ERights::None, resourceName, adminPK );
		AddRolePermission( rolePK, resourceName, ERights::All, ERights::None, adminPK );
	}

	//test deny
	//add group to group which it belongs to.
	//add role to role which it belongs to.
}