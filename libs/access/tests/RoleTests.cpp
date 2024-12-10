#include "gtest/gtest.h"
#include "../src/types/Role.h"
#include <jde/framework/io/json.h>
#include <jde/ql/ql.h>
#include "../src/types/Resource.h"
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class RoleTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};

	α RoleTests::SetUpTestCase()->void{
	}
	α RemoveRolePermission( RolePK rolePK, PermissionPK permissionPK, UserPK userPK )ε->jobject{
		let remove = Ƒ( "mutation removeRole( id:{}, permissionRight:{{id:{}}} )", rolePK, permissionPK );
		return QL::Query( remove, userPK );
	}
	α GetRolePermission( RolePK rolePK, sv resourceName, UserPK userPK )ε->jobject{
		let ql = Ƒ("query{{ role( id:{} ){{permissionRight{{id allowed denied resource(target:\"{}\",criteria:null)}} }} }}", rolePK, resourceName );
		return QL::Query( ql, userPK ); //{"role":{"member":{"id":1,"allowed":[],"denied":[]}}}
	}
	α GetRoleChild( RolePK parentRolePK, RolePK childRolePK, UserPK userPK )ε->jobject{
		let ql = Ƒ("query{{ role( id:{} ){{role(id:{}){{id target deleted}} }} }}", parentRolePK, childRolePK );
		return Json::FindDefaultObjectPath( QL::Query(ql, userPK), "role/role" );
	}

	α AddRolePermission( RolePK rolePK, sv resourceName, ERights allowed, ERights denied, UserPK userPK )ε->jobject{
		auto y = GetRolePermission( rolePK, resourceName, userPK );
		let member = Json::FindDefaultObjectPath( y, "role/permissionRight" );
		if( !member.empty() ){
			let existingAllowed = ToRights( Json::AsArray(member, "allowed") );
			let existingDenied = ToRights( Json::AsArray(member, "denied") );
			if( allowed!=existingAllowed || denied!=existingDenied ){
				let update = Ƒ( "mutation updatePermissionRight( id:{}, input:{{ allowed:{}, denied:{} }} )", GetId(member), underlying(allowed), underlying(denied) );
				QL::Query( update, userPK );
				y = GetRolePermission( rolePK, resourceName, userPK );
			}
		}
		else{
			let add = Ƒ( "{{ mutation addRole( id:{}, allowed:{}, denied:{}, resource:{{target:\"{}\"}} ) }}", rolePK, underlying(allowed), underlying(denied), resourceName );
			QL::Query( add, userPK );
			y = GetRolePermission( rolePK, resourceName, userPK );
		}
		return y;
	}
	α AddRoleMember( RolePK parentRolePK, RolePK childRolePK, UserPK userPK )ε->jobject{
		auto y = GetRoleChild( parentRolePK, childRolePK, userPK );
		if( y.empty() ){
			let add = Ƒ( "{{ mutation addRole( id:{}, role:{{id:{}}} ) }}", parentRolePK, childRolePK );
			QL::Query( add, userPK );
			y = GetRoleChild( parentRolePK, childRolePK, userPK );
		}
		return y;
	}

	TEST_F( RoleTests, Crud ){
		let pk = TestCrud( "role", "roleTest", GetRoot() );
		TestPurge( "role", pk, GetRoot() );
	}
	TEST_F( RoleTests, AddRemove ){
		//DS().Execute( "delete from role_members" );
		//DS().Execute( "delete from roles" );
		let rolePK = GetId( Get("role", "rolePermissionsTest", GetRoot()) );
		auto initial = AddRolePermission( rolePK, "users", ERights::All, ERights::None, GetRoot() );
		ASSERT_EQ( ToRights( Json::AsArrayPath(initial, "role/permissionRight/allowed") ), ERights::All );
		ASSERT_EQ( ToRights( Json::AsArrayPath(initial, "role/permissionRight/denied") ), ERights::None );

		RemoveRolePermission( rolePK, Json::AsNumber<PermissionPK>(initial, "role/permissionRight/id"), GetRoot() );
		auto roleMember = GetRolePermission( rolePK, "users", GetRoot() );
		ASSERT_TRUE( Json::FindDefaultObjectPath( roleMember, "role/permissionRight" ).empty() );

		AddRolePermission( rolePK, "users", ERights::Read, ERights::Update, GetRoot() );//purge with permissions.
		Purge( "role", rolePK, GetRoot() );
	}
}
