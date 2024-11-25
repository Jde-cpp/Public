#include "gtest/gtest.h"
#include "globals.h"
#include <jde/framework/str.h>
#include <jde/ql/ql.h>
#include <jde/db/meta/Table.h>
#include "../src/types/Resource.h"
#if unused
#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class PermissionTests : public ::testing::Test{
	};


	TEST_F( PermissionTests, Crud ){
		auto resourcePK = Json::AsNumber<PermissionPK>( SelectResource("identityGroups", GetRoot()), "id" );
		auto permission = GetPermission( resourcePK, ERights::Read, ERights::Update, GetRoot() );
		ASSERT_TRUE( permission.size() );
		let id = Json::AsNumber<PermissionPK>( permission, "id" );

		let update = Ƒ( "{{ mutation updatePermission( 'id':{}, 'input': {{'allowed': ['Read','Update'], 'denied':['None'] }}) }}", id );
		let updateJson = QL::Query( Str::Replace(update,'\'', '"'), 0 );
		permission = SelectPermission( resourcePK, GetRoot() );
		ASSERT_EQ( Json::AsArray( permission, "allowed" ).size(), 2 );
		ASSERT_EQ( Json::AsArray( permission, "denied" ).size(), 0 );

 		let purge = Ƒ( "{{ mutation purgePermission(\"id\":{}) }}", id );
		let purgeJson = QL::Query( purge, 0 );
		permission = SelectPermission( resourcePK, GetRoot() );
		ASSERT_TRUE( permission.empty() );
	}
}
#endif