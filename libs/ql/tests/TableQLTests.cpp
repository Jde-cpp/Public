//review3 #5:  three noexcept TableQL methods called throwing Boost.JSON accessors on client-shaped data, so a malformed
//request took the process down instead of being refused.  All three are still ι/Ι - the fix is to make them total, the way
//Input::OrderByJson already is (InputTests.OrderByJsonSurvivesNonStringMembers) - so these tests assert the survival
//behaviour rather than a throw.  Schema-free:  a system TableQL resolves no view, and none of the three touch one.
#include <gtest/gtest.h>
#include <jde/db/meta/Column.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/TableQL.h>

#define let const auto

namespace Jde::QL::Tests{
	Ω qlTable( sv jsonName, jobject args={} )ε->TableQL{
		const vector<sp<DB::AppSchema>> noSchemas;
		return TableQL{ string{jsonName}, move(args), ms<jobject>(), noSchemas, true };
	}
	Ω subTable( TableQL& parent, sv jsonName )ε->TableQL&{
		const vector<sp<DB::AppSchema>> noSchemas;
		parent.Tables.emplace_back( string{jsonName}, jobject{}, ms<jobject>(), noSchemas, true );
		return parent.Tables.back();
	}

	//(a) `{ profiles(filter:1){ id } }` - unauthenticated, and the criterion being added is the access-control row scope, so
	//it has to survive: with no filter object to put it in, it goes to Args, which is where AddFilter puts it anyway.
	TEST( TableQLTests, AddFilterSurvivesANonObjectFilterArg ){
		auto scalar = qlTable( "profiles", jobject{{"filter",1}} );
		scalar.AddFilter( "identity_id", jvalue{7} );
		ASSERT_TRUE( scalar.Args.contains("identity_id") ); //not dropped - it is what scopes the query.
		EXPECT_EQ( scalar.Args.at("identity_id").as_int64(), 7 );
		EXPECT_EQ( scalar.Args.at("filter").as_int64(), 1 ); //the junk is left for ToWhereClause to reject as an unknown column.

		auto object = qlTable( "profiles", jobject{{"filter",jobject{{"name","bob"}}}} );
		object.AddFilter( "identity_id", jvalue{7} );
		let& filter = object.Args.at("filter").as_object();
		EXPECT_TRUE( filter.contains("identity_id") ); //an object filter still gets it, as before.
		EXPECT_FALSE( object.Args.contains("identity_id") );

		auto none = qlTable( "profiles" );
		none.AddFilter( "identity_id", jvalue{7} );
		EXPECT_TRUE( none.Args.contains("identity_id") );
	}

	//(b) a mutation's scalar arg named like a subscribed sub-table (`createResource(…, resources:1)`) reaches TrimColumns
	//through LocalSubscriptions' fan-out, whose try/catch cannot see a terminate.
	TEST( TableQLTests, TrimColumnsSurvivesANonObjectSubTable ){
		auto ql = qlTable( "resourcesCreated" );
		ql.Columns.push_back( ColumnQL{"id"} );
		subTable( ql, "resources" ).Columns.push_back( ColumnQL{"id"} );

		let trimmed = ql.TrimColumns( jobject{ {"id",6}, {"resources",1} } );
		EXPECT_EQ( trimmed.at("id").as_int64(), 6 ); //the rest of the payload still goes out.
		EXPECT_FALSE( trimmed.contains("resources") ); //nothing to trim to the requested shape.

		let control = ql.TrimColumns( jobject{ {"id",6}, {"resources",jobject{{"id",6}}} } );
		EXPECT_EQ( control.at("resources").as_object().at("id").as_int64(), 6 );
	}

	//#17: the same hazard reached the way a client reaches it - through the parser rather than a hand-built TableQL - and with
	//the array shape the write-up names, which is what a mutation returning a list of ids under a subscribed sub-table sends.
	TEST( TableQLTests, TrimColumnsSurvivesAnArrayValuedSubTable ){
		const vector<sp<DB::AppSchema>> noSchemas;
		let ql = QL::ParseQuery( "status{ id groups{ id } }", {}, noSchemas ); //system table: resolves no view, so no schema is needed.
		let trimmed = ql.TrimColumns( jobject{ {"id",1}, {"groups",jarray{1,2}} } );
		EXPECT_EQ( trimmed.at("id").as_int64(), 1 );
		EXPECT_FALSE( trimmed.contains("groups") );
	}

	//(c) `{ permissionRights{ id resource resource{ id name deleted } } }` - the bare fk stem resolves to resources.name and
	//writes a string at "resource", then the sub-table of the same name wants an object there.
	TEST( TableQLTests, SetResultSurvivesAColumnAndSubTableSharingAName ){
		auto ql = qlTable( "permissionRights" );
		auto name = ms<DB::Column>( "name" );
		auto resourceId = ms<DB::Column>( "resource_id" );
		ql.Columns.push_back( ColumnQL{"resource", name} );
		subTable( ql, "resource" ).Columns.push_back( ColumnQL{"id", resourceId} );

		jobject o;
		ql.SetResult( o, name, DB::Value{"groups"} );      //the column half writes a string.
		EXPECT_EQ( o.at("resource").as_string(), "groups" );
		ql.SetResult( o, resourceId, DB::Value{6} );        //the sub-table half used to call as_object() on it.
		EXPECT_EQ( o.at("resource").as_string(), "groups" ); //still the string that was asked for; the sub-table is skipped.
	}

	//review3 #39: the same ι-plus-throwing-accessor shape, in the other direction - TransformResult(jarray&&) took `as_object()`
	//on the first element for a singular table, so anything else terminated the process rather than answering.  Latent (every
	//in-tree caller pushes objects), and total now: no row and a non-object row both come back as the empty object.
	TEST( TableQLTests, TransformResultSurvivesANonObjectFirstElement ){
		auto singular = qlTable( "setting" );  //singular: the branch that reaches [0].
		for( auto&& junk : {jarray{1}, jarray{"x"}, jarray{jarray{1}}, jarray{nullptr}} ){
			let y = singular.TransformResult( jarray{junk} );
			ASSERT_TRUE( y.is_object() ) << serialize( y );
			EXPECT_TRUE( y.as_object().empty() ) << serialize( y ); //the same answer as no row at all.
		}
	}
	//and what it must still do with the shapes it actually gets.
	TEST( TableQLTests, TransformResultUnwrapsASingularRowAndKeepsAPluralArray ){
		auto singular = qlTable( "setting" );
		let one = singular.TransformResult( jarray{jobject{{"id",7}}} );
		ASSERT_TRUE( one.is_object() );
		EXPECT_EQ( one.as_object().at("id").to_number<uint>(), 7u ); //unwrapped from the array.
		EXPECT_TRUE( singular.TransformResult(jarray{}).as_object().empty() );

		auto plural = qlTable( "settings" );
		let many = plural.TransformResult( jarray{jobject{{"id",7}}, 1} );
		ASSERT_TRUE( many.is_array() ); //plural never indexes, so a junk element is the caller's business.
		EXPECT_EQ( many.as_array().size(), 2u );
	}
}
