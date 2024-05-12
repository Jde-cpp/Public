#include "gtest/gtest.h"
#include "../../../../Framework/source/db/GraphQL.h"

#define var const auto
namespace Jde{
	static sp<Jde::LogTag> _logTag{ Logging::Tag( "tests" ) };

	class UserTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};
	
	α UserTests::SetUpTestCase()->void{
		UM::Configure();
	}
	
	TEST_F( UserTests, Fields ){
		var query = "{ __type(name: \"User\") { fields { name type { name kind ofType{name kind} } } }}";
		var json = DB::Query( query, 0 );
		TRACE( "{}", json.dump() );
		ASSERT_EQ( Str::Replace( json.dump(), '"', '\'' ), "{'data':{'__type':{'fields':[{'name':'id','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'ID'}}},{'name':'name','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'attributes','type':{'kind':0,'name':'UInt'}},{'name':'created','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'DateTime'}}},{'name':'updated','type':{'kind':0,'name':'DateTime'}},{'name':'deleted','type':{'kind':0,'name':'DateTime'}},{'name':'target','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'description','type':{'kind':0,'name':'String'}},{'name':'isGroup','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'Boolean'}}},{'name':'provider','type':{'kind':1,'name':'Provider'}},{'name':'password','type':{'kind':0,'name':'String'}}],'name':'User'}}}" );
		//                                                  {'data':{'__type':{'fields':[{'name':'id','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'ID'}}},{'name':'name','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'attributes','type':{'kind':0,'name':'UInt'}},{'name':'created','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'DateTime'}}},{'name':'updated','type':{'kind':0,'name':'DateTime'}},{'name':'deleted','type':{'kind':0,'name':'DateTime'}},{'name':'target','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'String'}}},{'name':'description','type':{'kind':0,'name':'String'}},{'name':'isGroup','type':{'kind':7,'name':null,'ofType':{'kind':0,'name':'Boolean'}}},{'name':'provider','type':{'kind':1,'name':'Provider'}},{'name':'password','type':{'kind':0,'name':'String'}}],'name':'User'}}}
	}

	TEST_F( UserTests, Crud ){
		var create = "{ mutation { createUser(  'input': {'target':'id','provider':1,'name':'name','description':'description'} ){id} } }";
		var createJson = DB::Query( Str::Replace(create, '\'', '"'), 0 );
		TRACE( "{}", createJson.dump() );
		var id = createJson["data"]["user"]["id"].get<int>();//{"data":{"user":{"id":7}}}

		var selectAll = "query{ users { id name attributes created updated deleted target description provider } }";
		var selectAllJson = DB::Query( selectAll, 0 );
		TRACE( "{}", selectAllJson.dump() );		

		var readGroups = "query{ users(filter:{isGroup:{ eq:true}}){ id name } }";
		var readGroupsJson = DB::Query( readGroups, 0 );
		TRACE( "{}", readGroupsJson.dump() );
		ASSERT_EQ( readGroupsJson["data"]["users"].size(), 0 );

		var read = "query{ user(filter:{target:{ eq:\"id\"}}){ id name attributes created updated deleted target description isGroup provider{ id name } } }";
		var readJson = DB::Query( read, 0 );
		TRACE( "{}", readJson.dump() );
		var readId = readJson["data"]["user"]["id"].get<int>();
		ASSERT_EQ( id, readId );

		var update = Jde::format( "{{ mutation {{ updateUser( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }} }}", readId, "newName" );
		var updateJson = DB::Query( update, 0 );
		TRACE( "{}", updateJson.dump() );		

		var del = Jde::format( "{{mutation {{ deleteUser(\"id\":{}) }} }}", readId );
		var deleteJson = DB::Query( del, 0 );
		TRACE( "{}", readJson.dump() );

		var purge = Jde::format( "{{mutation {{ purgeUser(\"id\":{}) }} }}", readId );
		var purgeJson = DB::Query( purge, 0 );
		TRACE( "{}", readJson.dump() );
	}
	
	TEST_F( UserTests, ProvidersSelect ){
		var readGroups = "query{ __type(name: \"Provider\") { enumValues { id name } } }";
		var readGroupsJson = DB::Query( readGroups, 0 );
		TRACE( "{}", readGroupsJson.dump() );
		ASSERT_TRUE( readGroupsJson["data"]["__type"]["enumValues"].size()>0 );
	}
	
}
