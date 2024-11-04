#include "gtest/gtest.h"
#include <jde/framework/io/json.h>
#include <jde/framework/str.h>
#include <jde/ql/ql.h>
#include "../src/types/Resource.h"
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	α GetPermission( ResourcePK resourcePK, ERights allowed, ERights denied, UserPK userPK )ε->jobject;
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
		let ql = Ƒ( "query< acl( identityId:{} )< identityId permission< id allowed denied resource(target:\"{}\")<deleted>> > >", identityPK, resourceTarget );
		let qlResult = QL::Query( Str::Replace(Str::Replace(ql,"<","{"), ">", "}"), 0 );
		return Json::FindDefaultObjectPath( qlResult, "data/acl" );
	}
	α GetAcl( IdentityPK identityPK, string resource, ERights allowed, ERights denied )ε->jobject{
		auto entry = SelectAcl( identityPK, resource );
		if( entry.empty() ){
			let resourceId = AsNumber<ResourcePK>( SelectResource(resource, true), "id" );
			let create = Ƒ( "mutation createAcl( input:{{ identityId:{}, permission:{{ allowed:{}, denied:{}, resource:{{id:{}}}}} }} )", identityPK, underlying(allowed), underlying(denied), resourceId );
			let createJson = QL::Query( create, 0 );
			entry = SelectAcl( identityPK, resource );
		}
		else{
			let existingAllowed = ToRights( Json::AsArrayPath(entry, "permission/allowed") );
			let existingDenied = ToRights( Json::AsArrayPath(entry, "permission/denied") );
			if( allowed!=existingAllowed || existingDenied!=denied ){
				let update = Ƒ( "mutation updatePermission( permissionId:{}, input:{{ allowed:{}, denied:{} }} )", Json::AsNumber<PermissionPK>(entry, "permission/id"), underlying(allowed), underlying(denied) );
				let updateJson = QL::Query( update, 0 );
				entry = SelectAcl( identityPK, resource );
			}
		}
		return entry;
	}
	α AclTests::SetUpTestCase()->void{
		SelectAcl( 1, "users" );
		array<string,10> users{ "intruder", "creator", "reader", "updater", "deleter", "purger", "admin", "subscriber", "executor", "root" };
		_resourceId =	AsNumber<UserPK>(SelectResource("users", true), "id" );
		auto allowed = ERights::None;
		for( let& user : users ){
			let& juser = Tests::Get( "user", user, GetRoot() );
			let userPK = GetId( juser );
			_usersPKs.emplace( user, userPK );
			let acl = GetAcl( userPK, "users", allowed, ERights::None );
			allowed = allowed==ERights::None
				? ERights::Create
				: allowed==ERights::Execute ? ERights::All : (ERights)(underlying(allowed)<<1);
			_users.emplace( user, acl );
		}
	}
	TEST_F( AclTests, DisabledPermission ){
		DS().Execute( "delete from identity_groups" );
		let resource = SelectResource( "identityGroups", GetRoot() );
		if( resource.at("deleted").is_null() )
			Delete( "resources", GetId(resource), GetRoot() );
		let intruderId = _usersPKs["intruder"];
		let groupId = TestCrud( "identityGroup", "DisabledPermission-Test-Group", intruderId );
		TestAdd( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"], _usersPKs["reader"]}, intruderId );
		TestRemove( "identityGroups", groupId, {_usersPKs["intruder"], _usersPKs["creator"]}, intruderId );
	}
	//enabled permissions.
	//test deleted user.
	//test in groups.
	//test permission in role.
	//test deny
	//test anonymous
}