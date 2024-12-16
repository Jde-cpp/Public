#include "gtest/gtest.h"
#include "globals.h"
#include <jde/ql/ql.h>
#include <jde/db/meta/Table.h>
#include "../src/types/Resource.h"

#define let const auto
namespace Jde::Access::Tests{
//	using namespace Json;
//	using namespace Tests;
	class ResourceTests : public ::testing::Test{
	};

	α selectResources( sv target, string filter, bool includeDeleted=false )->jarray{
		let targetFilter = target.size() ? Ƒ( ", target:\"{}\"", target ) : "";
		if( filter.size() )
			filter = Ƒ( ", criteria:\"{}\"", filter );
		let ql = Ƒ( "resources( schemaName:\"access\"{}{} ){{ id schemaName allowed denied name attributes created {} updated target description }}", targetFilter, filter, includeDeleted ? "deleted" : "" );
		let qlResult = QL::Query( ql, GetRoot() );
		return Json::AsArray( qlResult );
	}

	TEST_F( ResourceTests, CheckDefaults ){
		let ql = "resources( schemaName:\"access\", criteria:null ){ id allowed denied name attributes created deleted updated target description }";
		let qlResult = QL::Query( ql, GetRoot() );
		let& resources = Json::AsArray( qlResult );
		ASSERT_EQ( resources.size(), 4 ); //"users", identityGroups", "roles", "resources"
		constexpr ERights base = ERights::Create | ERights::Read | ERights::Update | ERights::Delete | ERights::Purge | ERights::Administer;
		Trace{ ELogTags::Test, "base={:x}"sv, underlying(base) };
		for( let& v : resources ){
			let& o = Json::AsObject( v );
			let target = Json::AsSV( o, "target" );
			auto allowed = ToRights( Json::AsArray(o, "allowed") );
			//let denied = ToRights( Json::AsArray(o, "denied") );
			auto expected = base;
			if( target=="users" )
				expected = base | ERights::Execute;
			ASSERT_EQ( expected, allowed ) << "target=" << target;
		}
	}

	TEST_F( ResourceTests, Crud ){
		constexpr sv target = "identityGroups";
		let userPK = GetId( GetUser("resourceTester", GetRoot()) );
		let filter = Ƒ( "userId:{{ eq: {} }}", userPK );
		auto resources = selectResources( target, filter );
		if( !resources.size() ){
			let& userTable = *GetTable( "users" );
			let create = Ƒ( "{{ mutation createResource( input:{{ schemaName:\"access\", name:\"creator\", target:\"{}\", criteria:\"{}\", rights:{} }} ) }}", target, filter, underlying(userTable.Operations) );
			let createJson = QL::Query( create, GetRoot() );
			resources = selectResources( target, filter );
			ASSERT_EQ( resources.size(), 1 );
		}
		let id = GetId( Json::AsObject(resources[0]) );

		let update = Ƒ( "{{ mutation updateResource( \"id\":{}, \"input\": {{\"allowed\": [\"Read\"] }}) }}", id );
		let updateJson = QL::Query( update, GetRoot() );
		let rights = selectResources(target, filter)[0].at("allowed").as_array();
		ASSERT_TRUE( rights.size()==1 );
		ASSERT_EQ( Json::AsSV(rights[0]), "Read" );

 		let del = Ƒ( "{{ mutation deleteResource(\"id\":{}) }}", id );
 		let deleteJson = QL::Query( del, GetRoot() );
		ASSERT_TRUE( selectResources(target, filter).empty() );
		ASSERT_TRUE( !selectResources(target, filter, true).empty() );

 		let restore = Ƒ( "{{ mutation restoreResource(\"id\":{}) }}", id );
 		let restoreJson = QL::Query( restore, GetRoot() );
		ASSERT_TRUE( !selectResources(target, filter).empty() );

 		let purge = Ƒ( "{{mutation purgeResource(\"id\":{}) }}", id );
 		ASSERT_TRUE( Tests::SelectGroup("groupTest", GetRoot(), true).empty() );
	}
}
