//ql-review3 #31: `__schema{ mutationType{…} }` could never answer.  LoadTables binds __schema to whatever table sorts first
//(access `acl`), which CustomQuery then claimed as an acl query - "query not implemented"; had it got past that,
//ColumnQL::QLType throws for users.password (VarBinary) and killed the document; and QuerySchema returned the *inner*
//mutationType, so a client reading data.__schema.mutationType.fields got a TypeError.
#include "gtest/gtest.h"
#include <jde/ql/types/MutationQL.h>
#include "globals.h"

#define let const auto

namespace Jde::Access::Tests{
	TEST( SchemaIntrospectionTests, MutationTypeAnswersWithItsWrapper ){
		let y = QL().QuerySync( "__schema{ mutationType{ name fields{ name args{ name defaultValue type{ name } } } } }", {}, GetRoot() );
		let& mutationType = Json::AsObject( y, "mutationType" );//the wrapper the client reads, not its contents.
		EXPECT_EQ( Json::AsSV(mutationType, "name"), "Mutation" );

		let& fields = Json::AsArray( mutationType, "fields" );
		ASSERT_FALSE( fields.empty() );
		flat_set<string> names;
		for( let& f : fields )
			names.emplace( Json::AsString(Json::AsObject(f), "name") );
		EXPECT_TRUE( names.contains("updateUser") ) << "users is a plain table - it has to advertise the crud verbs";
		EXPECT_TRUE( names.contains("purgeUser") );

		//users.password is VarBinary, which graphql has no spelling for:  the column is left out, the document survives.
		for( let& f : fields ){
			let& o = Json::AsObject( f );
			if( Json::AsSV(o, "name")!="updateUser" )
				continue;
			for( let& arg : Json::AsArray(o, "args") )
				EXPECT_NE( Json::AsSV(Json::AsObject(arg), "name"), "password" ) << serialize( o );
		}
	}

	//ql-review3 #41: the document advertised `insert{Type}`, which is not one of MutationQLStrings' ten verbs - and because
	//IsMutation("insertUser") is false, such a call was not even routed to LoadMutations;  it went to LoadTables as a query.
	//The `field["name"] = Ƒ("create{}")` above the lambda was the intended spelling and was never pushed.  Everything the
	//schema advertises has to be something the parser it advertises itself to will accept.
	TEST( SchemaIntrospectionTests, EveryAdvertisedMutationIsAVerbTheParserAccepts ){
		let y = QL().QuerySync( "__schema{ mutationType{ fields{ name } } }", {}, GetRoot() );
		let& fields = Json::AsArray( Json::AsObject(y, "mutationType"), "fields" );
		ASSERT_FALSE( fields.empty() );
		flat_set<string> names;
		for( let& f : fields )
			names.emplace( Json::AsString(Json::AsObject(f), "name") );
		for( let& name : names ){
			EXPECT_TRUE( QL::MutationQL::IsMutation(name) ) << name << " is not routed to LoadMutations at all";
			EXPECT_NO_THROW( QL::MutationQL::ParseCommand(name) ) << name;
		}
		EXPECT_TRUE( names.contains("createUser") ) << Str::Join( names, "," );
		EXPECT_FALSE( names.contains("insertUser") ) << Str::Join( names, "," );

		//Not this finding, and pinned as it behaves so a fix shows up here:  `acl` singularises to `Ac` - JsonName strips the
		//trailing 'l' as though it were a plural - so createAc/deleteAc/... are advertised.  They are legal verbs, which is why
		//the loop above passes, but ToPlural(FromJson("ac")) is "acs" and resolves no table.  A naming defect, not a verb one.
		EXPECT_TRUE( names.contains("deleteAc") ) << Str::Join( names, "," );
	}
}
