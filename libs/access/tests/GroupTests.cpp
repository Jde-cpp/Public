#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/ql/types/Introspection.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/framework/str.h>
#include "globals.h"

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Test };
	using namespace Json;
	using namespace Tests;
	class GroupTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};

	α GroupTests::SetUpTestCase()->void{
	}

	TEST_F( GroupTests, Fields ){
		//const QL::TableQL ql{ "" };
		let query = "{ __type(name: \"IdentityGroup\") { fields { name type { name kind ofType{name kind ofType{name kind ofType{name kind}}} } } } }";
		let actual = QL::Query( query, 0 );
		auto obj = Json::ReadJsonNet( Ƒ("{}/Public/libs/access/config/access-ql.jsonnet", OSApp::EnvironmentVariable("JDE_DIR").value_or("./")) );
		QL::Introspection intro{ move(obj) };
		QL::RequestQL request = QL::Parse( query );
		jobject expected;
		expected["__type"] = intro.Find("IdentityGroup")->ToJson( get<0>(request)[0].Tables[0] );
		ASSERT_EQ( serialize(actual), serialize(expected) );
	}

	TEST_F( GroupTests, Crud ){
		let group = Tests::GetGroup( "groupTest", GetRoot() );
		let id = Json::AsNumber<GroupPK>( group, "id" );

 		let update = Ƒ( "{{ mutation updateIdentityGroup( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }} }}", id, "newName" );
 		let updateJson = QL::Query( update, GetRoot() );
		ASSERT_TRUE( AsSV(Tests::SelectGroup("groupTest", GetRoot()), "name")=="newName" );

 		let del = Ƒ( "{{mutation deleteIdentityGroup(\"id\":{}) }}", id );
 		let deleteJson = QL::Query( del, GetRoot() );
		ASSERT_TRUE( Tests::SelectGroup("groupTest", GetRoot()).empty() );
		ASSERT_TRUE( !Tests::SelectGroup("groupTest", GetRoot(), true).empty() );

 		PurgeGroup( id, GetRoot() );
 		ASSERT_TRUE( Tests::SelectGroup("groupTest", GetRoot(), true).empty() );
	}
	TEST_F( GroupTests, AddRemove ){
		let hrManagers = Json::AsNumber<UserPK>( Tests::GetGroup("HR-Managers", GetRoot()), "id" );
		let manager = Json::AsNumber<UserPK>( Tests::GetUser("manager", GetRoot()), "id" );
		Tests::AddToGroup( hrManagers, {manager}, GetRoot() );
		constexpr sv ql = "query{{ identityGroup(id:{}){{ members{{id name}} }} }}";
		ASSERT_EQ( Json::AsArrayPath(QL::Query( Ƒ(ql, hrManagers), GetRoot() ), "identityGroup/members" ).size(), 1 );

		let hr = Json::AsNumber<UserPK>( Tests::GetGroup("HR", GetRoot()), "id" );
		let associate = Json::AsNumber<UserPK>(Tests::GetUser("associate", GetRoot()), "id");
		Tests::AddToGroup( hr, {hrManagers, associate}, GetRoot() );
		let members = QL::Query( Ƒ(ql, hr), GetRoot() );
		let array = Json::AsArrayPath( members, "identityGroup/members" );
		ASSERT_EQ( array.size(), 2 );

		constexpr sv userQL = "query{{ user(id:{}){{ identityGroups{{id name}} }} }}";
		ASSERT_EQ( Json::AsArrayPath(QL::Query( Ƒ(userQL, manager), GetRoot() ), "user/identityGroups" ).size(), 1 );

		Tests::RemoveFromGroup( hr, {hrManagers, associate}, GetRoot() );
		Tests::RemoveFromGroup( hrManagers, {manager}, GetRoot() );
		Tests::PurgeGroup( hr, GetRoot() );
		Tests::PurgeGroup( hrManagers, GetRoot() );
		Tests::PurgeUser( associate, GetRoot() );
		Tests::PurgeUser( manager, GetRoot() );
	}
	//add
	//remove
	//user groups
}
