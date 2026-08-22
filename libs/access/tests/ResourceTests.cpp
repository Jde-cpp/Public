#include "gtest/gtest.h"
#include "globals.h"
#include <jde/ql/QLAwait.h>
#include <jde/db/meta/Table.h>
#include <jde/access/types/Resource.h>

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
		let ql = Ƒ( "resources( schemaName:\"access\"{}{} ){{ id schemaName allowed name attributes created {} updated target description }}", targetFilter, filter, includeDeleted ? "deleted" : "" );
		return QL().QuerySync<jarray>( ql, {}, GetRoot() );
	}

	TEST_F( ResourceTests, CheckDefaults ){
		let ql = "resources( schemaName:\"access\", criteria:null ){ id allowed name attributes created deleted updated target description }";
		let& resources = QL().QuerySync<jarray>( ql, {}, GetRoot() );
		ASSERT_EQ( resources.size(), 5 ); //"users", "members", "roles", "resources", "provider_types"
		constexpr ERights base = ERights::Create | ERights::Read | ERights::Update | ERights::Delete | ERights::Purge | ERights::Administer;
		TRACET( ELogTags::Test, "base={:x}", underlying(base) );
		for( let& v : resources ){
			let& o = Json::AsObject( v );
			let target = Json::AsSV( o, "target" );
			auto allowed = ToRights( Json::AsArray(o, "allowed") );
			auto expected = base;
			if( target=="users" )
				expected = base | ERights::Execute;
			else if( target=="resources" )
				expected = ERights::Delete;
			ASSERT_EQ( expected, allowed ) << "target=" << target;
		}
	}

	TEST_F( ResourceTests, Crud ){
		constexpr sv target = "members";
		let userPK = GetId( GetUser("resourceTester", GetRoot()) );
		let filter = Ƒ( "userId:{{ eq: {} }}", userPK );
		auto resources = selectResources( target, filter );
		if( !resources.size() ){
			let& userTable = *GetTable( "users" );
			let create = Ƒ( "mutation createResource( schemaName:\"access\", name:\"creator\", target:\"{}\", criteria:\"{}\", rights:{} )", target, filter, underlying(userTable.Operations) );
			let createJson = QL().QuerySync<jvalue>( create, {}, GetRoot() );
			resources = selectResources( target, filter );
			ASSERT_EQ( resources.size(), 1 );
		}
		let id = GetId( Json::AsObject(resources[0]) );

		let update = Ƒ( "mutation updateResource( \"id\":{}, \"allowed\": [\"Read\"] )", id );
		let updateJson = QL().QuerySync<jvalue>( update, {}, GetRoot() );
		let rights = selectResources(target, filter)[0].at("allowed").as_array();
		ASSERT_TRUE( rights.size()==1 );
		ASSERT_EQ( Json::AsSV(rights[0]), "Read" );

 		let del = Ƒ( "mutation deleteResource(\"id\":{})", id );
 		let deleteJson = QL().QuerySync<jvalue>( del, {}, GetRoot() );
		ASSERT_TRUE( selectResources(target, filter).empty() );
		ASSERT_TRUE( !selectResources(target, filter, true).empty() );

 		let restore = Ƒ( "mutation restoreResource(\"id\":{})", id );
 		let restoreJson = QL().QuerySync<jvalue>( restore, {}, GetRoot() );
		ASSERT_TRUE( !selectResources(target, filter).empty() );

 		let purge = Ƒ( "{{mutation purgeResource(\"id\":{}) }}", id );
 		ASSERT_TRUE( Tests::SelectGroup("groupTest", GetRoot(), true).empty() );
	}

	//ql-review3 #48: the insert-side twin.  getEnumValue had its own flags loop that `continue`d past a non-string element, so
	//an all-numeric array - which no name lookup could ever match - wrote flags==0 with no error, and the resource was silently
	//unusable (allowed=0 hides it from the permission table).  Both paths share one parser now.
	TEST_F( ResourceTests, NumericFlagsAreAnErrorOnInsertToo ){
		constexpr sv schema{ "qlFlagTests" };
		constexpr sv target{ "flagNumeric" };
		let select = Ƒ( R"(resources( schemaName:"{}", target:"{}" ){{ id allowed }})", schema, target );
		EXPECT_THROW( QL().QuerySync<jvalue>(Ƒ(R"(mutation createResource( schemaName:"{0}", name:"{1}", target:"{1}", allowed:[1,2] ))", schema, target), {}, GetRoot()), Exception );
		EXPECT_TRUE( QL().QuerySync<jarray>(select, {}, GetRoot()).empty() ) << "the row was created with allowed=0";

		//and the same array through the update path, which has always refused it - the two now refuse it in the same words.
		QL().QuerySync<jvalue>( Ƒ(R"(mutation createResource( schemaName:"{0}", name:"{1}", target:"{1}", allowed:["Read"] ))", schema, target), {}, GetRoot() );
		auto rows = QL().QuerySync<jarray>( select, {}, GetRoot() );
		ASSERT_EQ( rows.size(), 1u );
		let id = GetId( Json::AsObject(rows[0]) );
		EXPECT_THROW( QL().QuerySync<jvalue>(Ƒ(R"(mutation updateResource( id:{}, allowed:[1,2] ))", id), {}, GetRoot()), Exception );
		EXPECT_EQ( serialize(Json::AsObject(QL().QuerySync<jarray>(select, {}, GetRoot())[0]).at("allowed")), R"(["Read"])" );
		Purge( "resource", id, GetRoot() );
	}
	//the control:  a flags array of names still inserts, which is the shape everything in tree sends.
	TEST_F( ResourceTests, NamedFlagsStillInsert ){
		constexpr sv schema{ "qlFlagTests" };
		constexpr sv target{ "flagNamed" };
		QL().QuerySync<jvalue>( Ƒ(R"(mutation createResource( schemaName:"{0}", name:"{1}", target:"{1}", allowed:["Read","Update"] ))", schema, target), {}, GetRoot() );
		let rows = QL().QuerySync<jarray>( Ƒ(R"(resources( schemaName:"{}", target:"{}" ){{ id allowed }})", schema, target), {}, GetRoot() );
		ASSERT_EQ( rows.size(), 1u );
		EXPECT_EQ( Json::AsArray(Json::AsObject(rows[0]), "allowed").size(), 2u );
		Purge( "resource", GetId(Json::AsObject(rows[0])), GetRoot() );
	}
	TEST_F( ResourceTests, UnknownFlagNameIsAnErrorNotAWipe ){
		constexpr sv schema{ "qlFlagTests" };
		constexpr sv target{ "flagTypo" };
		let select = Ƒ( R"(resources( schemaName:"{}", target:"{}" ){{ id allowed }})", schema, target );
		if( QL().QuerySync<jarray>(select, {}, GetRoot()).empty() )
			QL().QuerySync<jvalue>( Ƒ(R"(mutation createResource( schemaName:"{0}", name:"{1}", target:"{1}", allowed:["Read"] ))", schema, target), {}, GetRoot() );
		auto rows = QL().QuerySync<jarray>( select, {}, GetRoot() );
		ASSERT_EQ( rows.size(), 1u );
		let id = GetId( Json::AsObject(rows[0]) );
		let before = serialize( Json::AsObject(rows[0]).at("allowed") );

		EXPECT_THROW( QL().QuerySync<jvalue>(Ƒ(R"(mutation updateResource( "id":{}, "allowed":["Reed"] ))", id), {}, GetRoot()), Exception );
		let after = QL().QuerySync<jarray>( select, {}, GetRoot() );
		ASSERT_EQ( after.size(), 1u );
		EXPECT_EQ( serialize(Json::AsObject(after[0]).at("allowed")), before ); //unchanged - the update never ran, rather than running with 0.
	}
}
