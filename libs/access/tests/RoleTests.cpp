#include "gtest/gtest.h"
#include "../src/types/Role.h"
#include <jde/framework/io/json.h>
#include "../src/types/Resource.h"
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	α GetPermission( ResourcePK resourcePK, ERights allowed, ERights denied, UserPK userPK )ε->jobject;
	using namespace Json;
	class RoleTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};

	α RoleTests::SetUpTestCase()->void{
	}

	TEST_F( RoleTests, Crud ){
		let role = Tests::Get( "role", "roleTest", GetRoot() );
		let id = AsNumber<RolePK>( role, "id" );
		Tests::TestUpdateName( "role", id, GetRoot() );
		Tests::TestDelete( "role", id, GetRoot() );
		Tests::TestPurge( "role", id, GetRoot() );
	}
	TEST_F( RoleTests, AddRemove ){
//		DS().Execute( "delete from role_permissions" );
		let resourcePK = Json::AsNumber<ResourcePK>( SelectResource("users", GetRoot()), "id" );
		let permissionId = Json::AsNumber<ResourcePK>( GetPermission(resourcePK, ERights::None, ERights::None, GetRoot()), "id" );
		let roleId = Json::AsNumber<RolePK>( Tests::Get("role", "rolePermissionsTest", GetRoot()), "id" );
		Tests::Add( *GetTable("role_permissions"), roleId, {permissionId}, GetRoot() );
		auto role = Tests::Get( "role", "rolePermissionsTest", GetRoot(), {"permissions{id allowed denied}"} );
		ASSERT_EQ( Json::AsArrayPath(role, "permissions").size(), 1 );

		Tests::Remove( *GetTable("role_permissions"), roleId, {permissionId}, GetRoot() );
		role = Tests::Get( "role", "rolePermissionsTest", GetRoot(), {"permissions{id allowed denied}"} );
		ASSERT_EQ( Json::AsArrayPath(role, "permissions").size(), 0 );

		Tests::Purge( "role", roleId, GetRoot() );
	}
}
