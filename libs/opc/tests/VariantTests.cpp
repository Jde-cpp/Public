//Variant is the persistence path:  ToUAJson() writes one UA-JSON string per element into the db, ToUAValues() reads
//them back, ToArrayDims()/ArrayDimString() carry the shape alongside.  These are the round trips reviews/opc-review2.md
//says would have caught most of the review.
#include <gtest/gtest.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Variant.h>

#define let const auto

namespace Jde::Opc::Tests{
	Ω scalarVariant( const void* value, const UA_DataType& type )ι->UA_Variant{
		UA_Variant v{};
		UA_Variant_setScalarCopy( &v, value, &type );
		return v;
	}
	Ω arrayVariant( const void* values, uint count, const UA_DataType& type )ι->UA_Variant{
		UA_Variant v{};
		UA_Variant_setArrayCopy( &v, values, count, &type );
		return v;
	}
	Ω stored( const vector<string>& uaJson )ι->flat_map<uint,string>{
		flat_map<uint,string> y;
		for( uint i=0; i<uaJson.size(); ++i )
			y.emplace( i, uaJson[i] );
		return y;
	}

	TEST( VariantTests, EmptyAndScalar ){
		EXPECT_TRUE( Variant( UA_Variant{} ).IsNull() );
		EXPECT_TRUE( Variant( UA_Variant{} ).ToUAJson().empty() );

		const UA_Int32 i{ 42 };
		Variant scalar{ scalarVariant(&i, UA_TYPES[UA_TYPES_INT32]) };
		EXPECT_FALSE( scalar.IsNull() );
		EXPECT_TRUE( scalar.IsScalar() );
		EXPECT_EQ( serialize(scalar.ToJson(true)), "42" );
	}

	TEST( VariantTests, StatusCodeScalar ){
		Variant v{ (StatusCode)UA_STATUSCODE_BADNODEIDUNKNOWN };
		EXPECT_TRUE( v.IsScalar() );
		EXPECT_EQ( v.type, &UA_TYPES[UA_TYPES_STATUSCODE] );
	}

	TEST( VariantTests, MoveEmptiesTheSource ){
		const UA_Int32 i{ 42 };
		Variant v{ scalarVariant(&i, UA_TYPES[UA_TYPES_INT32]) };
		auto raw = v.Move();
		EXPECT_TRUE( v.IsNull() );
		EXPECT_EQ( raw.type, &UA_TYPES[UA_TYPES_INT32] );
		UA_Variant_clear( &raw ); //Move() made us the owner.
	}

	//The json inference used when a node has no declared DataType.  Note "double" is the only dataType string honoured -
	//see DISABLED_R2_8 below.
	TEST( VariantTests, FromJsonInfersTheType ){
		EXPECT_EQ( Variant( jvalue{true}, "" ).type, &UA_TYPES[UA_TYPES_BOOLEAN] );
		EXPECT_EQ( Variant( jvalue{"abc"}, "" ).type, &UA_TYPES[UA_TYPES_STRING] );
		EXPECT_EQ( Variant( jvalue{1.5}, "" ).type, &UA_TYPES[UA_TYPES_DOUBLE] );
		EXPECT_EQ( Variant( jvalue{5}, "" ).type, &UA_TYPES[UA_TYPES_INT64] );
		EXPECT_EQ( Variant( jvalue{5u}, "" ).type, &UA_TYPES[UA_TYPES_UINT64] );
		EXPECT_EQ( Variant( jvalue{5}, "double" ).type, &UA_TYPES[UA_TYPES_DOUBLE] );
		EXPECT_EQ( serialize(Variant( jvalue{"abc"}, "" ).ToJson(true)), R"("abc")" );
	}

	TEST( VariantTests, ScalarToJsonUsesTheOpcShapeForNodeIds ){
		let n = NodeId{ 2, 5002 };
		Variant v{ scalarVariant(static_cast<const UA_NodeId*>(&n), UA_TYPES[UA_TYPES_NODEID]) };
		let j = v.ToJson( true );
		EXPECT_EQ( j.at("ns").to_number<int>(), 2 );
		EXPECT_EQ( j.at("i").to_number<int>(), 5002 );
	}

	TEST( VariantTests, ArrayToJsonIsPerElement ){
		const UA_Int32 values[]{ 1, 2, 3 };
		Variant v{ arrayVariant(values, 3u, UA_TYPES[UA_TYPES_INT32]) };
		EXPECT_FALSE( v.IsScalar() );
		EXPECT_EQ( serialize(v.ToJson(true)), "[1,2,3]" );
	}

	TEST( VariantTests, ArrayDims ){
		let dims = Variant::ToArrayDims( "2,3" );
		ASSERT_EQ( get<1>(dims), 2u );
		EXPECT_EQ( get<0>(dims)[0], 2u );
		EXPECT_EQ( get<0>(dims)[1], 3u );

		auto data = UA_Array_new( 6, &UA_TYPES[UA_TYPES_INT32] );
		ASSERT_NE( data, nullptr );
		Variant v{ 0, {6u, data}, dims, UA_TYPES[UA_TYPES_INT32] }; //takes ownership of both allocations.
		EXPECT_EQ( v.ArrayDimString(), "2,3" );

		let none = Variant::ToArrayDims( "" );
		EXPECT_EQ( get<0>(none), nullptr );
		EXPECT_EQ( get<1>(none), 0u );
		EXPECT_TRUE( Variant( UA_Variant{} ).ArrayDimString().empty() );
	}

	//The persistence round trip.  ToUAJson emits one raw UA-JSON token per element and ToUAValues decodes them back;
	//the strings have to reach the db unmodified.
	TEST( VariantTests, UaJsonArrayRoundTrip ){
		const UA_Int32 values[]{ 1, 2, 3 };
		Variant original{ arrayVariant(values, 3u, UA_TYPES[UA_TYPES_INT32]) };
		let json = original.ToUAJson();
		ASSERT_EQ( json.size(), 3u );
		EXPECT_EQ( json[0], "1" );
		EXPECT_EQ( json[2], "3" );

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], stored(json) );
		ASSERT_NE( data, nullptr );
		ASSERT_EQ( count, 3u );
		Variant round{ 0, {count, data}, Variant::ToArrayDims("3"), UA_TYPES[UA_TYPES_INT32] };
		EXPECT_EQ( serialize(round.ToJson(true)), "[1,2,3]" );
		EXPECT_EQ( round.ArrayDimString(), "3" );
	}

	TEST( VariantTests, UaJsonScalarRoundTrip ){
		let text = ToUV( "tag value" );
		Variant original{ scalarVariant(&text, UA_TYPES[UA_TYPES_STRING]) };
		let json = original.ToUAJson();
		ASSERT_EQ( json.size(), 1u );
		EXPECT_EQ( json[0], R"("tag value")" );

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_STRING], stored(json) );
		ASSERT_NE( data, nullptr );
		EXPECT_EQ( count, 0u ); //a single value decodes as a scalar - arrayLength 0, data non-null.
		Variant round{ 0, {count, data}, Variant::ToArrayDims(""), UA_TYPES[UA_TYPES_STRING] };
		EXPECT_TRUE( round.IsScalar() );
		EXPECT_EQ( serialize(round.ToJson(true)), R"("tag value")" );
	}

	//reviews/opc-review2.md #1:  ToUAValues catches every decode failure and returns {0,nullptr} with no log, so a row it
	//cannot read is indistinguishable from "no value".  Both OpcServer insert sites store serialize(array[i]), which
	//json-quotes an already-encoded token - an Int32 element 5 goes in as "5" and comes back null on every restart.  The
	//fix is at the call sites (store the raw token) plus an ERR here; this pins the contract both halves depend on.
	TEST( VariantTests, ToUAValuesNeedsRawUaJson ){
		flat_map<uint,string> raw;
		raw.emplace( 0u, "5" );
		let [rawCount, rawData] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(raw) );
		ASSERT_NE( rawData, nullptr );
		EXPECT_EQ( rawCount, 0u );
		EXPECT_EQ( *(const UA_Int32*)rawData, 5 );
		UA_delete( rawData, &UA_TYPES[UA_TYPES_INT32] );

		flat_map<uint,string> quoted;
		quoted.emplace( 0u, serialize(jvalue{"5"}) ); //exactly what serialize(array[i]) writes today.
		let [quotedCount, quotedData] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(quoted) );
		EXPECT_EQ( quotedCount, 0u );
		EXPECT_EQ( quotedData, nullptr ) << R"(an Int32 column holding "5" must not decode)";
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#8: only the literal "double" is honoured; every other declared dataType falls through to json inference, so
	//{dataType:"int", value:5} yields an Int64 variant against an Int32 DataType attribute and UA_Server_addVariableNode
	//rejects it with BadTypeMismatch.  The names are OpcServer's DT() vocabulary, which is where the mapping belongs.
	TEST( VariantTests, DISABLED_R2_8_DeclaredDataTypeWins ){
		EXPECT_EQ( Variant( jvalue{5}, "int" ).type, &UA_TYPES[UA_TYPES_INT32] );
		EXPECT_EQ( Variant( jvalue{5}, "uint" ).type, &UA_TYPES[UA_TYPES_UINT32] );
		EXPECT_EQ( Variant( jvalue{5}, "byte" ).type, &UA_TYPES[UA_TYPES_BYTE] );
		EXPECT_EQ( Variant( jvalue{5}, "double" ).type, &UA_TYPES[UA_TYPES_DOUBLE] );
	}

	//#10: a one-element array collapses to a scalar ("size==1 would be an array") while the caller still applies the
	//persisted arrayDimensions, producing scalar data with arrayDimensionsSize 1 - open62541 answers BadTypeMismatch and
	//one-element arrays do not survive a restart.  The stored dims, not the element count, decide.
	TEST( VariantTests, DISABLED_R2_10_OneElementArrayStaysAnArray ){
		flat_map<uint,string> one;
		one.emplace( 0u, "5" );
		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(one) );
		ASSERT_NE( data, nullptr );
		ASSERT_EQ( count, 1u );
		UA_Array_delete( data, 1, &UA_TYPES[UA_TYPES_INT32] );
	}

	//#11: open62541 uses a non-empty outBuf as a hard limit, so the fixed 2096-byte buffer in uaJsonString caps every
	//value - a large but perfectly valid string/ByteString/ExtensionObject throws BadEncodingLimitsExceeded instead of
	//encoding.  Pass an empty UA_String and let the encoder size it (which also drops a 2 KB malloc per element).
	TEST( VariantTests, DISABLED_R2_11_LargeValueEncodes ){
		let text = string( 4096, 'x' );
		let ua = ToUV( text );
		Variant v{ scalarVariant(&ua, UA_TYPES[UA_TYPES_STRING]) };
		vector<string> json;
		ASSERT_NO_THROW( json = v.ToUAJson() );
		ASSERT_EQ( json.size(), 1u );
		EXPECT_EQ( json[0].size(), text.size()+2 ); //the value plus its two quotes.
	}
}
