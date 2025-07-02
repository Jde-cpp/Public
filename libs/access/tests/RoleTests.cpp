#include "gtest/gtest.h"
#include <jde/ql/QLAwait.h>
#include <jde/access/types/Role.h>
#include <jde/access/types/Resource.h>
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
	α RemoveRolePermission( RolePK rolePK, PermissionPK permissionPK, UserPK userPK )ε->jvalue{
		let remove = Ƒ( "mutation removeRole( id:{}, permissionRight:{{id:{}}} )", rolePK, permissionPK );
		return QL::QuerySync<jvalue>( remove, userPK );
	}
	α GetRolePermission( RolePK rolePK, sv resourceName, UserPK executer )ε->jobject{
		let ql = Ƒ("role( id:{} ){{permissionRight{{id allowed denied resource(target:\"{}\",criteria:null)}} }}", rolePK, resourceName );
		let role = QL::QuerySync( ql, executer ); //{"role":{"member":{"id":1,"allowed":[],"denied":[]}}}
		return Json::FindDefaultObjectPath( role, "permissionRight" );
	}
	α GetRoleChild( RolePK parentRolePK, RolePK childRolePK, UserPK userPK )ε->jobject{
		let ql = Ƒ("role( id:{} ){{role(id:{}){{id target deleted}} }}", parentRolePK, childRolePK );
		return Json::FindDefaultObjectPath( QL::QuerySync(ql, userPK), "role" );
	}

	α AddRolePermission( RolePK rolePK, sv resourceName, ERights allowed, ERights denied, UserPK userPK )ε->jobject{
		auto permission = GetRolePermission( rolePK, resourceName, userPK );
		if( !permission.empty() ){
			let existingAllowed = ToRights( Json::AsArray(permission, "allowed") );
			let existingDenied = ToRights( Json::AsArray(permission, "denied") );
			if( allowed!=existingAllowed || denied!=existingDenied ){
				let update = Ƒ( "mutation updatePermissionRight( id:{}, allowed:{}, denied:{} )", GetId(permission), underlying(allowed), underlying(denied) );
				QL::QuerySync( update, userPK );
				permission = GetRolePermission( rolePK, resourceName, userPK );
			}
		}
		else{
			let add = Ƒ( "mutation addRole( id:{}, permissionRight:{{allowed:{}, denied:{}, resource:{{target:\"{}\"}}}} )", rolePK, underlying(allowed), underlying(denied), resourceName );
			QL::QuerySync<jvalue>( add, userPK );
			permission = GetRolePermission( rolePK, resourceName, userPK );
		}
		return permission;
	}
	α AddRoleMember( RolePK parentRolePK, RolePK childRolePK, UserPK userPK )ε->jobject{
		auto y = GetRoleChild( parentRolePK, childRolePK, userPK );
		if( y.empty() ){
			let add = Ƒ( "mutation addRole( id:{}, role:{{id:{}}} )", parentRolePK, childRolePK );
			QL::QuerySync<jvalue>( add, userPK );
			y = GetRoleChild( parentRolePK, childRolePK, userPK );
		}
		return y;
	}

	TEST_F( RoleTests, Crud ){
		let pk = TestCrud( "role", "roleTest", GetRoot() );
		TestPurge( "role", pk, GetRoot() );
	}

	Ω getRole( str target, UserPK executer )ε->jobject{ return Get("role", target, executer); }

	TEST_F( RoleTests, AddRemove ){
		let rolePK = GetId( getRole("rolePermissionsTest", GetRoot()) );
		auto initial = AddRolePermission( rolePK, "users", ERights::All, ERights::None, GetRoot() );
		ASSERT_EQ( ToRights( Json::AsArrayPath(initial, "allowed") ), ERights::All );
		ASSERT_EQ( ToRights( Json::AsArrayPath(initial, "denied") ), ERights::None );

		RemoveRolePermission( rolePK, GetId(initial), GetRoot() );
		auto roleMember = GetRolePermission( rolePK, "users", GetRoot() );
		ASSERT_TRUE( roleMember.empty() );

		AddRolePermission( rolePK, "users", ERights::Read, ERights::Update, GetRoot() );//purge with permissions.
		Purge( "role", rolePK, GetRoot() );
	}

	TEST_F( RoleTests, Recursion ){
		const RolePK aRole{ GetId(getRole("roleRecursionA", GetRoot())) };
		const RolePK bRole{ GetId(getRole("roleRecursionB", GetRoot())) };
		AddRoleMember( aRole, {bRole}, GetRoot() );
		const RolePK cRole{ GetId(getRole("roleRecursionC", GetRoot())) };
		AddRoleMember( bRole, {cRole}, GetRoot() );

		const RolePK dRole{ GetId(getRole("roleRecursionD", GetRoot())) };
		EXPECT_THROW( AddRoleMember( dRole, {dRole}, GetRoot() ), IException );
		AddRoleMember( cRole, {dRole}, GetRoot() );
		EXPECT_THROW( AddRoleMember( dRole, {aRole}, GetRoot() ), IException );
		//TODO test implement deleted roles.
	}
}