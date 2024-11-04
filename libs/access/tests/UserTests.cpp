#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/db/Database.h>
#include <jde/access/usings.h>
#include "globals.h"

#define let const auto
namespace Jde::Access{
	using namespace Json;
	using namespace Tests;

	class UserTests : public ::testing::Test{
	protected:
	};
	std::condition_variable_any cv;
	std::shared_mutex mtx;


	TEST_F( UserTests, Fields ){
		let query = "{ __type(name: \"User\") { fields { name type { name kind ofType{name kind} } } }}";
		let json = QL::Query( query, 0 );
		let actual = Str::Replace( serialize(json), '"', '\'' );
		let expected = "{'data':{'__type':{'name':'User','fields':[{'name':'id','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'ID','kind':'SCALAR'}}},{'name':'name','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'String','kind':'SCALAR'}}},{'name':'provider','type':{'name':'Provider','kind':'ENUM'}},{'name':'target','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'String','kind':'SCALAR'}}},{'name':'attributes','type':{'name':'UInt','kind':'SCALAR'}},{'name':'created','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'DateTime','kind':'SCALAR'}}},{'name':'updated','type':{'name':'DateTime','kind':'SCALAR'}},{'name':'deleted','type':{'name':'DateTime','kind':'SCALAR'}},{'name':'description','type':{'name':'String','kind':'SCALAR'}},{'name':'isGroup','type':{'name':null,'kind':'NON_NULL','ofType':{'name':'Boolean','kind':'SCALAR'}}},{'name':'loginName','type':{'name':'String','kind':'SCALAR'}},{'name':'modulus','type':{'name':'String','kind':'SCALAR'}},{'name':'exponent','type':{'name':'UInt','kind':'SCALAR'}}]}}}";
		ASSERT_EQ( actual, expected );
	}

	TEST_F( UserTests, Crud ){
		const string target{ "crud" };
		let existingUser = GetUser( target, GetRoot() );
		auto id = FindNumberPath<UserPK>( existingUser, "id" ).value_or( 0 );
		ASSERT_NE( id, 0 );

		let update = Ƒ( "{{ mutation updateUser( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }}", id, "newName" );
		let updateJson = QL::Query( update, 0 );
		ASSERT_TRUE( AsSV(Tests::GetUser(target, GetRoot()), "name")=="newName" );

		let del = Ƒ( "{{ mutation deleteUser(\"id\":{}) }}", id );
		let deleteJson = QL::Query( del, 0 );
		ASSERT_TRUE( Tests::SelectUser(target, GetRoot()).empty() );
		ASSERT_TRUE( !Tests::GetUser(target, GetRoot(), true).empty() );

		PurgeUser( id, GetRoot() );
		ASSERT_TRUE( Tests::SelectUser(target, GetRoot(), true).empty() );
	}

	TEST_F( UserTests, MultipleUsersSelect ){
		let a = Json::AsNumber<UserPK>( GetUser("MultipleUsersA", GetRoot()), "id" );
		let b = Json::AsNumber<UserPK>( GetUser("MultipleUsersB", GetRoot()), "id" );
		let q = Ƒ( "query{{ users(id:[{},{}]){{id loginName provider}} }}", a, b );
		auto j = QL::Query( q, GetRoot() );
		PurgeUser( a, GetRoot() );
		PurgeUser( b, GetRoot() );
	}
	TEST_F( UserTests, ProvidersSelect ){
		let readGroups = "query{ __type(name: \"Provider\") { enumValues { id name } } }";
		let readGroupsJson = QL::Query( readGroups, 0 );
		ASSERT_TRUE( AsArrayPath(readGroupsJson, "data/__type/enumValues").size()>0 );
	}
}