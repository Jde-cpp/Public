#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
#include <jde/ql/ql.h>
#include "../src/types/Resource.h"
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class AclTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
		static flat_map<string,jobject> _users;
		static flat_map<string,uint> _usersPKs;
		static ResourcePK _resourceId;
	};
	flat_map<string,jobject> AclTests::_users;
	flat_map<string,uint> AclTests::_usersPKs;
	ResourcePK AclTests::_resourceId;

	α SelectAcl( IdentityPK identityPK, string resourceTarget )ε->jobject{
		let ql = Ƒ( "query< acl( identityId:{} )< identityId permissionRights< id allowed denied resource(target:\"{}\")<deleted>> > >", identityPK, resourceTarget );
		//let ql = Ƒ( "query< permissionRight< id allowed denied acl( identityId:{} ) resource(target:\"{}\")<deleted>> > >", identityPK, resourceTarget );
		let qlResult = QL::Query( Str::Replace(Str::Replace(ql,"<","{"), ">", "}"), 0 );
		return Json::FindDefaultObject( qlResult, "permissionRight" );
	}
	α GetAcl( IdentityPK identityPK, string resource, ERights allowed, ERights denied )ε->jobject{
		auto entry = SelectAcl( identityPK, resource );
		if( entry.empty() ){
			let resourceId = AsNumber<ResourcePK>( SelectResource(resource, true), "id" );
			let create = Ƒ( "mutation createAcl( input:{{ identityId:{}, permission:{{ allowed:{}, denied:{}, resource:{{id:{}}}}} }} )", identityPK, underlying(allowed), underlying(denied), resourceId );
			let createJson = QL::Query( create, GetRoot() );
			entry = SelectAcl( identityPK, resource );
		}
		else{
			let existingAllowed = ToRights( Json::AsArray(entry, "allowed") );
			let existingDenied = ToRights( Json::AsArray(entry, "denied") );
			if( allowed!=existingAllowed || existingDenied!=denied ){
				let update = Ƒ( "mutation updatePermissionRight( id:{}, input:{{ allowed:{}, denied:{} }} )", Json::AsNumber<PermissionPK>(entry, "id"), underlying(allowed), underlying(denied) );
				let updateJson = QL::Query( update, 0 );
				entry = SelectAcl( identityPK, resource );
			}
		}
		return entry;
	}

	α AclTests::SetUpTestCase()->void{
		SelectAcl( 1, "users" );
		array<string,10> users{ "intruder", "creator", "reader", "updater", "deleter", "purger", "admin", "subscriber", "executor", "root" };
		let resourceTarget = "identityGroups";
		_resourceId =	AsNumber<UserPK>(SelectResource(resourceTarget, true), "id" );
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
		let intruderId = _usersPKs["intruder"];
		let groupId = TestCrud( "identityGroup", "DisabledPermission-Test-Group", intruderId );
		TestAdd( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"], _usersPKs["reader"]}, intruderId );
		TestRemove( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"]}, intruderId );
		TestPurge( "identityGroups", groupId, intruderId );
	}
	TEST_F( AclTests, EnabledPermissions ){
		let resource = SelectResource( "identityGroups", GetRoot() );
		if( !resource.at("deleted").is_null() )
			Restore( "resources", GetId(resource), GetRoot() );
		let intruderId = _usersPKs["intruder"];
		let groupId = TestUnauthCrud( "identityGroup", "DeniedPermission-Test", intruderId );
		TestUnauthAddRemove( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"], _usersPKs["reader"]}, intruderId );
		TestUnauthPurge( "identityGroups", groupId, intruderId );
	}
	//test giving users permissions.
	//test deleted user.
	//test user in groups.
	//test group in groups.
	//test permission in role.
	//test role in role.
	//test deny
	//test anonymous
	//add group to group which it belongs to.
	//add role to role which it belongs to.
}