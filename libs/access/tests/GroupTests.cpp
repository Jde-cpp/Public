#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/ql/types/Introspection.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/TableQL.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/framework/str.h>
#include "globals.h"

#define let const auto
namespace Jde::Access::Tests{
	using namespace Json;
	class GroupTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};

	α GroupTests::SetUpTestCase()->void{
	}
	Ω IsMember( str target, GroupPK child, UserPK executer )ε->bool{
		let group = SelectGroup( target, executer, true );
		if( auto members = FindArray(group, "members"); members ){
			for( let& member : *members ){
				if( GetId(AsObject(member))==child.Value )
					return true;
			}
		}
		return false;
	}
	TEST_F( GroupTests, Fields ){
		//const QL::TableQL ql{ "" };
		let query = "{ __type(name: \"IdentityGroup\") { fields { name type { name kind ofType{name kind ofType{name kind ofType{name kind}}} } } } }";
		let actual = QL::Query( query, GetRoot() );
		auto obj = Json::ReadJsonNet( Ƒ("{}/Public/libs/access/config/access-ql.jsonnet", OSApp::EnvironmentVariable("JDE_DIR").value_or("./")) );
		QL::Introspection intro{ move(obj) };
		QL::RequestQL request = QL::Parse( query );
		jobject expected = intro.Find("IdentityGroup")->ToJson( get<0>(request)[0].Tables[0] );
		ASSERT_EQ( serialize(actual), serialize(expected) );
	}

	TEST_F( GroupTests, Crud ){
		let group = GetGroup( "groupTest", GetRoot() );
		let id = GetId( group );

 		let update = Ƒ( "{{ mutation updateIdentityGroup( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }} }}", id, "newName" );
 		let updateJson = QL::Query( update, GetRoot() );
		ASSERT_TRUE( AsSV(SelectGroup("groupTest", GetRoot()), "name")=="newName" );

 		let del = Ƒ( "{{mutation deleteIdentityGroup(\"id\":{}) }}", id );
 		let deleteJson = QL::Query( del, GetRoot() );
		ASSERT_TRUE( SelectGroup("groupTest", GetRoot()).empty() );
		ASSERT_TRUE( !SelectGroup("groupTest", GetRoot(), true).empty() );

 		PurgeGroup( {id}, GetRoot() );
 		ASSERT_TRUE( SelectGroup("groupTest", GetRoot(), true).empty() );
	}
	TEST_F( GroupTests, AddRemove ){
		const GroupPK hrManagers{ GetId(GetGroup("HR-Managers", GetRoot())) };
		const UserPK manager{ GetId(GetUser("manager", GetRoot())) };
		AddToGroup( hrManagers, {manager}, GetRoot() );
		constexpr sv ql = "identityGroup(id:{}){{ members{{id name}} }}";
		ASSERT_EQ( Json::AsArrayPath(QL::QueryObject( Ƒ(ql, hrManagers.Value), GetRoot() ), "members" ).size(), 1 );

		const GroupPK hr{ GetId( GetGroup("HR", GetRoot()) ) };
		const UserPK associate{ GetId( GetUser("associate", GetRoot()) ) };
		AddToGroup( {hr}, {hrManagers, associate}, GetRoot() );
		let members = QL::QueryObject( Ƒ(ql, hr.Value), GetRoot() );
		let array = Json::AsArrayPath( members, "members" );
		ASSERT_EQ( array.size(), 2 );

		constexpr sv userQL = "user(id:{}){{ identityGroups{{id name}} }}";
		ASSERT_EQ( Json::AsArrayPath(QL::QueryObject(Ƒ(userQL, manager.Value), GetRoot()), "identityGroups" ).size(), 1 );

		RemoveFromGroup( hr, {hrManagers, associate}, GetRoot() );
		RemoveFromGroup( hrManagers, {manager}, GetRoot() );
		PurgeGroup( hr, GetRoot() );
		PurgeGroup( hrManagers, GetRoot() );
		PurgeUser( associate, GetRoot() );
		PurgeUser( manager, GetRoot() );
	}

	TEST_F( GroupTests, Recursion ){
		const GroupPK groupA{ GetId(GetGroup("groupA", GetRoot())) };
		const GroupPK groupB{ GetId(GetGroup("groupB", GetRoot())) };
		if( !IsMember( "groupA", groupB, GetRoot()) )
			AddToGroup( groupA, {groupB}, GetRoot() );
		const GroupPK groupC{ GetId(GetGroup("groupC", GetRoot())) };
		if( !IsMember( "groupB", groupC, GetRoot()) )
			AddToGroup( groupB, {groupC}, GetRoot() );

		const GroupPK groupD{ GetId(GetGroup("groupD", GetRoot())) };
		EXPECT_THROW( AddToGroup( groupD, {groupD}, GetRoot() ), IException );
		if( !IsMember( "groupC", groupD, GetRoot()) )
			AddToGroup( groupC, {groupD}, GetRoot() );
		EXPECT_THROW( AddToGroup( groupD, {groupA}, GetRoot() ), IException );
		//TODO test implement deleted groups.
	}
}
