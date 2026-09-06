#include <gtest/gtest.h>
#include <jde/db/db.h>
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>

namespace Jde::DB::Tests{
	//db-refactor B4: Table absorbed View.  FindOwnColumn is the former View::FindColumn - this table's columns only, where
	//`id` names a lone surrogate key - and FindColumn is that, then the extended table's.  Neither needs a schema: Extends
	//is wired by hand here, the way Initialize would resolve the json placeholder.
	static auto parse( sv json )->jobject{ return boost::json::parse( json ).as_object(); }

	TEST( TableTests, FindOwnColumnStopsAtThisTable ){
		auto identities = ms<Table>( "identities", parse(R"({"columns":{"identityId":{"sk":0,"type":"UInt"},"name":{},"deleted":{"nullable":true}}})") );
		Table users{ "users", parse(R"({"columns":{"identityId":{"sk":0,"type":"UInt"},"target":{}},"extends":"identities"})") };
		ASSERT_TRUE( users.Extends );
		EXPECT_EQ( users.Extends->Name, "identities" ); //the ctor's placeholder, resolved by Initialize.
		users.Extends = identities;

		EXPECT_EQ( users.FindOwnColumn("target"), users.Columns[1] );
		EXPECT_FALSE( users.FindOwnColumn("name") );                   //identities' column: never through Extends.
		EXPECT_EQ( users.FindColumn("name"), identities->Columns[1] ); //FindColumn chains.
		EXPECT_FALSE( users.FindColumn("nothing") );
		EXPECT_FALSE( users.FindOwnColumn("deleted") );
		EXPECT_TRUE( users.FindColumn("deleted") );
	}

	TEST( TableTests, IdNamesTheLoneSurrogateKey ){
		Table users{ "users", parse(R"({"columns":{"identityId":{"sk":0,"type":"UInt"},"target":{}}})") };
		ASSERT_EQ( users.SurrogateKeys.size(), 1u );
		EXPECT_EQ( users.FindOwnColumn("id"), users.SurrogateKeys[0] ); //no column is called "id" - the alias resolves to the sk.
		EXPECT_EQ( users.FindColumn("id"), users.SurrogateKeys[0] );
		EXPECT_EQ( users.FindOwnColumn("identity_id"), users.SurrogateKeys[0] ); //json name -> db name.

		Table pair{ "pair", parse(R"({"columns":{"a":{"sk":0},"b":{"sk":1}}})") };
		EXPECT_FALSE( pair.FindOwnColumn("id") ); //two surrogate keys: "id" is ambiguous, so no alias.
		EXPECT_FALSE( Table{"placeholder"}.FindColumn("id") ); //no columns at all.
	}
}
