//review3 #32:  every test in this directory stopped at the tokenizer or at the static Parser::ParseArgs - nothing ran a whole
//document through QL::Parse, so LoadTables/LoadMutations/LoadSubscriptions/LoadUnsubscriptions had no direct coverage at all and
//the shapes only a *document* can express (two mutations in one block, an alias, an arg list across lines, unsubscribe) were
//unguarded.  Schema-free by design:  every table named here is a system name (status, logs), so TableQL/MutationQL resolve no
//view and nothing opens a data source.
#include <gtest/gtest.h>
#include <jde/ql/ql.h>
#include <jde/ql/types/Parser.h>
#include <jde/ql/types/Subscription.h>
#include "UnitSchema.h"

#define let const auto

namespace Jde::QL::Tests{
	static const vector<sp<DB::AppSchema>> _noSchemas;
	Ω parse( string text, jobject variables={} )ε->RequestQL{ return QL::Parse( move(text), move(variables), _noSchemas ); }

	//R1 (fixed 2026-07-16, never pinned):  vars is loop-invariant, so LoadMutations copies the sp instead of moving it - a moved-from
	//`vars` left the 2nd mutation with a null Variables, and Input::FindPtr dereferences it the moment an arg is a $variable.
	//Two parameterized mutations in one block is the only shape that shows it, and no test in the repo had one.
	TEST( ParseDocumentTests, TwoMutationsInOneBlockBothKeepTheVariables ){
		let request = parse( "mutation { updateStatus(id:$a) updateStatus(id:$b) }", jobject{{"a",1},{"b",2}} );
		ASSERT_TRUE( request.IsMutation() );
		let& mutations = request.Mutations();
		ASSERT_EQ( mutations.size(), 2u );
		ASSERT_TRUE( mutations[1].Variables ); //the null the copy prevents - AsNumber below would dereference it.
		EXPECT_EQ( mutations[0].Variables.get(), mutations[1].Variables.get() ); //one object, shared: both see every variable.
		EXPECT_EQ( mutations[0].AsNumber<uint>("id"), 1u );
		EXPECT_EQ( mutations[1].AsNumber<uint>("id"), 2u );
		EXPECT_EQ( mutations[1].Type, EMutationQL::Update );
	}

	//#19 end to end on the query side (ParserTests covers the mutation side):  a client that pretty-prints its arg list across
	//lines used to get `(1)boost.json - syntax error` from the whole document.
	TEST( ParseDocumentTests, MultiLineArgListOnAQuery ){
		let request = parse( "logs(\n a:1\n){ id }" );
		ASSERT_TRUE( request.IsQueries() );
		ASSERT_EQ( request.Queries().size(), 1u );
		let& logs = request.Queries()[0];
		EXPECT_EQ( logs.JsonName, "logs" );
		EXPECT_EQ( logs.AsNumber<uint>("a"), 1u );
		ASSERT_EQ( logs.Columns.size(), 1u );
		EXPECT_EQ( logs.Columns[0].JsonName, "id" );
	}

	//Q12/#43: an argument list written where a column belongs was taken literally - `status{ (schema:$s) id }` came back as the
	//four columns '(', 'schema:$s', ')' and 'id', and the predicate was nowhere.  Three access subscriptions sent exactly this
	//shape, which is why the guard and their relocation had to land together (EventsSubscribeAwait.cpp:50-51).
	TEST( ParseDocumentTests, ArgListInsideASelectionSetThrows ){
		for( let text : {"{ status{ (schema:$s) id } }", "{ status{ id ) } }"} ){
			try{
				parse( string{text}, jobject{{"s","x"}} );
				ADD_FAILURE() << "'" << text << "' parsed";
			}
			catch( const Exception& e ){
				EXPECT_NE( string{e.what()}.find("argument list where a column belongs"), string::npos ) << e.what();
			}
		}
		//the boundary the guard deliberately stops at: an arg list with a *name* in front of it is a sub-table, not junk - which
		//is how `permissionRight{ id resource(schema:$schemas) }` is meant to be read, and why only a bare '(' can be the error.
		let named = parse( "{ status{ id sub(a:1) } }" );
		let& status = named.Queries()[0];
		EXPECT_EQ( status.Columns.size(), 1u );
		ASSERT_EQ( status.Tables.size(), 1u );
		EXPECT_EQ( status.Tables[0].JsonName, "sub" );
		EXPECT_EQ( status.Tables[0].AsNumber<uint>("a"), 1u );
	}
	//the one delimiter the selection set does reject today, and the control that the same args on the table still work.
	TEST( ParseDocumentTests, CommaBetweenColumnsThrows ){
		try{
			parse( "{ status{ id , name } }" );
			ADD_FAILURE() << "parsed";
		}
		catch( const Exception& e ){
			EXPECT_NE( string{e.what()}.find("separate columns"), string::npos ) << e.what();
		}
	}
	TEST( ParseDocumentTests, ArgListOnTheTableStillParses ){
		let request = parse( "{ status(schema:$s){ id } }", jobject{{"s","x"}} );
		let& status = request.Queries()[0];
		EXPECT_EQ( status.As<jstring>("schema"), "x" );
		ASSERT_EQ( status.Columns.size(), 1u );
		EXPECT_EQ( status.Columns[0].JsonName, "id" );
	}

	//LoadTables' do/while:  a document may name more than one table, and the loop's `Peek().size()` continuation had no test.
	TEST( ParseDocumentTests, TwoTablesInOneDocument ){
		let request = parse( "{ status{ up } logs{ id } }" );
		ASSERT_EQ( request.Queries().size(), 2u );
		EXPECT_EQ( request.Queries()[0].JsonName, "status" );
		EXPECT_EQ( request.Queries()[1].JsonName, "logs" );
	}

	//An alias is what the reply is keyed by (TableQL::ReturnName), so a client that renames a table gets its own name back.
	TEST( ParseDocumentTests, AliasIsCarried ){
		let request = parse( "{ s: status{ up } }" );
		ASSERT_EQ( request.Queries().size(), 1u );
		let& status = request.Queries()[0];
		EXPECT_EQ( status.Alias, "s" );
		EXPECT_EQ( status.JsonName, "status" ); //the alias is not the table.
		EXPECT_EQ( status.ReturnName(), "s" );
		EXPECT_EQ( parse("{ status{ up } }").Queries()[0].ReturnName(), "status" ); //and with no alias, the table's own name.
	}

	//#30:  the unsubscribe leg of the document.  Nothing anywhere parsed one.
	TEST( ParseDocumentTests, UnsubscribeRoundTrips ){
		let request = parse( "unsubscribe{ id:[1,2,3] }" );
		EXPECT_FALSE( request.IsQueries() );
		EXPECT_FALSE( request.IsMutation() );
		EXPECT_FALSE( request.IsSubscription() );
		let& ids = request.UnSubscribes();
		ASSERT_EQ( ids.size(), 3u );
		EXPECT_EQ( ids[0], 1u );
		EXPECT_EQ( ids[2], 3u );
		EXPECT_EQ( parse("unsubscribe{ id:[7] }").UnSubscribes().size(), 1u ); //a single id is still an array.
	}

	//#1's parser half:  `mutation create()` is a *valid parse* that resolves no table - the name is empty, so no lookup can be
	//made.  That the crud ops refuse it instead of dereferencing the null is MutationAwaitTests.EmptyTableNameThrows.
	TEST( ParseDocumentTests, EmptyMutationNameParsesWithNoTable ){
		let request = parse( "mutation create()" );
		ASSERT_TRUE( request.IsMutation() );
		let& m = request.Mutations()[0];
		EXPECT_TRUE( m.JTableName().empty() );
		EXPECT_FALSE( m.DBTable );
		EXPECT_TRUE( m.TableName().empty() );
	}

	//#6 (ql-review2, fixed 2026-07-07, never pinned):  client-supplied subscription ids all defaulted to 0 and collided, so the
	//server assigns from NextId when the client names none.  The generated ids have to stay above the client/requestId range.
	TEST( ParseDocumentTests, ClientSubscriptionIdIsHonoredAndRemovedFromTheFilter ){
		let request = parse( "subscription LogCreated{ logCreated(subscriptionId:5){ id } }" );
		ASSERT_TRUE( request.IsSubscription() );
		let& sub = request.Subscriptions()[0];
		EXPECT_EQ( sub.TableName, "logs" );
		EXPECT_EQ( sub.Type, EMutationQL::Create );
		EXPECT_EQ( sub.Id, 5u );
		EXPECT_FALSE( sub.Fields.Args.contains("subscriptionId") ); //erased, or it would filter logs.log_id=5.
	}
	TEST( ParseDocumentTests, NoClientSubscriptionIdLeavesItForTheServer ){
		EXPECT_EQ( parse("subscription LogCreated{ logCreated{ id } }").Subscriptions()[0].Id, 0u ); //LocalQL assigns when !Id.
	}
	TEST( ParseDocumentTests, NextIdIsMonotonicAndAboveTheClientRange ){
		let first = Subscription::NextId();
		let second = Subscription::NextId();
		EXPECT_GT( second, first );
		EXPECT_GE( first, 0x80000000u ); //above the client-supplied & requestId ranges - the collision #6 was about.
	}

	//#42, at the document level: `startups` is an ordinary table whose name happens to start with the verb `start`, so the whole
	//query was handed to LoadMutations and came back "Expected '(' vs i @ '13' to start function".  The schema is needed here -
	//routing it correctly means it now reaches LoadTables, which resolves the view.
	TEST( ParseDocumentTests, ATableWhoseNameStartsWithAVerbIsAQuery ){
		let schema = schemas();
		let request = QL::Parse( "{ startups{ id name } }", {}, schema );
		ASSERT_TRUE( request.IsQueries() );
		ASSERT_EQ( request.Queries().size(), 1u );
		EXPECT_EQ( request.Queries()[0].JsonName, "startups" );
		EXPECT_EQ( request.Queries()[0].Columns.size(), 2u );
	}
	//and the mutation on the same table still routes as one - the fix must not cost the verb its own tables.
	TEST( ParseDocumentTests, AMutationOnThatTableStillRoutesAsAMutation ){
		let schema = schemas();
		let request = QL::Parse( "mutation createStartup( name:\"x\" )", {}, schema );
		ASSERT_TRUE( request.IsMutation() );
		EXPECT_EQ( request.Mutations()[0].TableName(), "startups" );
	}
}
