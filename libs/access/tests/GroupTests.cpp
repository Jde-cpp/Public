#include "gtest/gtest.h"
#include <jde/ql/ql.h>
#include <jde/framework/str.h>

#define let const auto
namespace Jde{
	constexpr ELogTags _tags{ ELogTags::Test };

	class GroupTests : public ::testing::Test{
	protected:
		Ω SetUpTestCase()->void;
	};

	α GroupTests::SetUpTestCase()->void{
	}

	TEST_F( GroupTests, Fields ){
		let query = "{ __type(name: \"Group\") { fields { name type { name kind ofType{name kind ofType{name kind ofType{name kind}}} } } }}";
		let json = QL::Query( query, 0 );
		Trace{ _tags, "{}", serialize(json) };
		ASSERT_EQ( Str::Replace( serialize(json), '"', '\'' ), "{'data':{'__type':{'fields':[{'name':'id','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'ID'}}},{'name':'name','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'String'}}},{'name':'attributes','type':{'kind':'SCALAR','name':'PositiveInt'}},{'name':'created','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'DateTime'}}},{'name':'updated','type':{'kind':'SCALAR','name':'DateTime'}},{'name':'deleted','type':{'kind':'SCALAR','name':'DateTime'}},{'name':'target','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'ID'}}},{'name':'description','type':{'kind':'SCALAR','name':'String'}},{'name':'provider','type':{'kind':'ENUM','name':'Provider'}},{'name':'members','type':{'kind':'LIST','name':null,'ofType':{'kind':'NON_NULL','name':null,'ofType':{'kind':'UNION','name':'Entity'}}}}],'name':'Group'}}}" );
//		                                                  {'data':{'__type':{'fields':[{'name':'id','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'ID'}}},{'name':'name','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'String'}}},{'name':'attributes','type':{'kind':'SCALAR','name':'PositiveInt'}},{'name':'created','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'DateTime'}}},{'name':'updated','type':{'kind':'SCALAR','name':'DateTime'}},{'name':'deleted','type':{'kind':'SCALAR','name':'DateTime'}},{'name':'target','type':{'kind':'NON_NULL','name':null,'ofType':{'kind':'SCALAR','name':'ID'}}},{'name':'description','type':{'kind':'SCALAR','name':'String'}},{'name':'provider','type':{'kind':'ENUM','name':'Provider'}},{'name':'members','type':{'kind':'LIST','name':'',  'ofType':{'kind':'NON_NULL','name':null,'ofType':{'kind':'UNION','name':'Entity'}}}}],'name':'Group'}}}
		//{'name':'members','type':{'kind':6,'name':'Entity'}}],'name':'Group'}}}
		//{'name':'members','type':{'kind':6,'name':null,'ofType':{'kind':7,'name':null,ofType':{'kind':3,'name':'Entity'}}}}
		//enum class FieldKind : uint8{ Scalar=0, Object=1, Interface=2, Union=3, Enum=4, InputObject=5, List=6, NonNull=7 };
	}

	TEST_F( GroupTests, Crud ){
		// let create = "{ mutation { createUser(  'input': {'target':'id','provider':1,'name':'name','description':'description'} ){id} } }";
		// let createJson = DB::Query( Str::Replace(create, '\'', '"'), 0 );
		// Trace{ _tags, "{}", createJson.dump() };
		// let id = createJson["data"]["user"]["id"].get<int>();//{"data":{"user":{"id":7}}}

		// let selectAll = "query{ groups { id name attributes created updated deleted target description provider members } }";
		// let selectAllJson = DB::Query( selectAll, 0 );
		// Trace{ _tags, "{}", selectAllJson.dump() };

// 		let readGroups = "query{ users(filter:{isGroup:{ eq:true}}){ id name } }";
// 		let readGroupsJson = DB::Query( readGroups, 0 );
// 		Trace{ _tags, "{}", readGroupsJson.dump() };
// 		ASSERT_EQ( readGroupsJson["data"]["users"].size(), 0 );

// 		let read = "query{ user(filter:{target:{ eq:\"id\"}}){ id name attributes created updated deleted target description isGroup provider{ id name } } }";
// 		let readJson = DB::Query( read, 0 );
// 		Trace{ _tags, "{}", readJson.dump() };
// 		let readId = readJson["data"]["user"]["id"].get<int>();
// //		ASSERT_EQ( id, readId );

// 		let update = Jde::format( "{{ mutation {{ updateUser( \"id\":{}, \"input\": {{\"name\":\"{}\"}} ) }} }}", readId, "newName" );
// 		let updateJson = DB::Query( update, 0 );
// 		Trace{ _tags, "{}", updateJson.dump() };

// 		let del = Jde::format( "{{mutation {{ deleteUser(\"id\":{}) }} }}", readId );
// 		let deleteJson = DB::Query( del, 0 );
// 		Trace{ _tags, "{}", readJson.dump() };

// 		let purge = Jde::format( "{{mutation {{ purgeUser(\"id\":{}) }} }}", readId );
// 		let purgeJson = DB::Query( purge, 0 );
// 		Trace{ "{}", readJson.dump() };
	}
}
