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

	//A null dataType means "no declared DataType attribute" - infer from the json kind.
	TEST( VariantTests, FromJsonInfersTheType ){
		EXPECT_EQ( Variant( jvalue{true}, nullptr ).type, &UA_TYPES[UA_TYPES_BOOLEAN] );
		EXPECT_EQ( Variant( jvalue{"abc"}, nullptr ).type, &UA_TYPES[UA_TYPES_STRING] );
		EXPECT_EQ( Variant( jvalue{1.5}, nullptr ).type, &UA_TYPES[UA_TYPES_DOUBLE] );
		EXPECT_EQ( Variant( jvalue{5}, nullptr ).type, &UA_TYPES[UA_TYPES_INT64] );
		EXPECT_EQ( Variant( jvalue{5u}, nullptr ).type, &UA_TYPES[UA_TYPES_UINT64] );
		EXPECT_EQ( serialize(Variant( jvalue{"abc"}, nullptr ).ToJson(true)), R"("abc")" );
		EXPECT_THROW( Variant( jvalue{}, nullptr ), Exception ); //nothing to infer from.
	}

	//#8 (fixed): only the literal "double" used to be honoured; every other declared dataType fell through to json
	//inference, so {dataType:"int", value:5} yielded an Int64 variant against an Int32 DataType attribute and
	//UA_Server_addVariableNode rejected it with BadTypeMismatch.  The declared type now decides, and the value is
	//decoded toward it rather than merely labelled with it.
	TEST( VariantTests, DeclaredDataTypeWins ){
		EXPECT_EQ( Variant( jvalue{5}, &UA_TYPES[UA_TYPES_INT32] ).type, &UA_TYPES[UA_TYPES_INT32] );
		EXPECT_EQ( Variant( jvalue{5}, &UA_TYPES[UA_TYPES_UINT32] ).type, &UA_TYPES[UA_TYPES_UINT32] );
		EXPECT_EQ( Variant( jvalue{5}, &UA_TYPES[UA_TYPES_BYTE] ).type, &UA_TYPES[UA_TYPES_BYTE] );
		EXPECT_EQ( Variant( jvalue{5}, &UA_TYPES[UA_TYPES_INT16] ).type, &UA_TYPES[UA_TYPES_INT16] );
		EXPECT_EQ( Variant( jvalue{5}, &UA_TYPES[UA_TYPES_FLOAT] ).type, &UA_TYPES[UA_TYPES_FLOAT] );
		EXPECT_EQ( Variant( jvalue{5}, &UA_TYPES[UA_TYPES_DOUBLE] ).type, &UA_TYPES[UA_TYPES_DOUBLE] );

		//the value has to arrive as that type, not just be tagged with it.
		Variant i32{ jvalue{5}, &UA_TYPES[UA_TYPES_INT32] };
		ASSERT_TRUE( i32.IsScalar() );
		EXPECT_EQ( *(const UA_Int32*)i32.data, 5 );
		EXPECT_EQ( serialize(i32.ToJson(true)), "5" );

		Variant f{ jvalue{1.5}, &UA_TYPES[UA_TYPES_FLOAT] };
		EXPECT_FLOAT_EQ( *(const UA_Float*)f.data, 1.5f );

		//and a value the declared type cannot hold is rejected rather than silently retyped.
		EXPECT_THROW( Variant( jvalue{"abc"}, &UA_TYPES[UA_TYPES_INT32] ), Exception );
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

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], stored(json), true );
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

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_STRING], stored(json), false );
		ASSERT_NE( data, nullptr );
		EXPECT_EQ( count, 0u ); //a single value decodes as a scalar - arrayLength 0, data non-null.
		Variant round{ 0, {count, data}, Variant::ToArrayDims(""), UA_TYPES[UA_TYPES_STRING] };
		EXPECT_TRUE( round.IsScalar() );
		EXPECT_EQ( serialize(round.ToJson(true)), R"("tag value")" );
	}

	TEST( VariantTests, ToUAValuesNeedsRawUaJson ){
		flat_map<uint,string> raw;
		raw.emplace( 0u, "5" );
		let [rawCount, rawData] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(raw), false );
		ASSERT_NE( rawData, nullptr );
		EXPECT_EQ( rawCount, 0u );
		EXPECT_EQ( *(const UA_Int32*)rawData, 5 );
		UA_delete( rawData, &UA_TYPES[UA_TYPES_INT32] );

		flat_map<uint,string> quoted;
		quoted.emplace( 0u, serialize(jvalue{"5"}) ); //exactly what serialize(array[i]) writes today.
		let [quotedCount, quotedData] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(quoted), false );
		EXPECT_EQ( quotedCount, 0u );
		EXPECT_EQ( quotedData, nullptr ) << R"(an Int32 column holding "5" must not decode)";
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#10 (fixed): a one-element array collapsed to a scalar ("size==1 would be an array") while the caller still applied
	//the persisted arrayDimensions, producing scalar data with arrayDimensionsSize 1 - open62541 answers BadTypeMismatch,
	//so one-element arrays did not survive a restart.  The stored dims, not the element count, decide.
	TEST( VariantTests, OneElementArrayStaysAnArray ){
		const UA_Int32 one[]{ 5 };
		Variant original{ arrayVariant(one, 1u, UA_TYPES[UA_TYPES_INT32]) };
		ASSERT_FALSE( original.IsScalar() );
		let json = original.ToUAJson();
		ASSERT_EQ( json.size(), 1u ); //indistinguishable from a scalar by count alone - hence the isArray argument.

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], stored(json), true );
		ASSERT_NE( data, nullptr );
		ASSERT_EQ( count, 1u );
		Variant round{ 0, {count, data}, Variant::ToArrayDims("1"), UA_TYPES[UA_TYPES_INT32] };
		EXPECT_FALSE( round.IsScalar() ) << "scalar data carrying arrayDimensions is what open62541 rejects";
		EXPECT_EQ( serialize(round.ToJson(true)), "[5]" );
		EXPECT_EQ( round.ArrayDimString(), "1" );
	}

	//...and the same single stored element still reloads as a scalar when no dims were persisted.
	TEST( VariantTests, OneElementScalarStaysAScalar ){
		flat_map<uint,string> one;
		one.emplace( 0u, "5" );
		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(one), false );
		ASSERT_NE( data, nullptr );
		EXPECT_EQ( count, 0u );
		Variant round{ 0, {count, data}, Variant::ToArrayDims(""), UA_TYPES[UA_TYPES_INT32] };
		EXPECT_TRUE( round.IsScalar() );
		EXPECT_EQ( serialize(round.ToJson(true)), "5" );
	}

	//#11 (fixed): open62541 uses a non-empty outBuf as a hard limit, so the fixed 2096-byte buffer in uaJsonString capped
	//every value - a large but perfectly valid string/ByteString/ExtensionObject threw BadEncodingLimitsExceeded instead
	//of encoding.  uaJsonString now passes an empty UA_String and lets the encoder size it, which also drops a 2 KB
	//malloc per element.
	TEST( VariantTests, LargeValueEncodes ){
		let text = string( 4096, 'x' );
		let ua = ToUV( text );
		Variant v{ scalarVariant(&ua, UA_TYPES[UA_TYPES_STRING]) };
		vector<string> json;
		ASSERT_NO_THROW( json = v.ToUAJson() );
		ASSERT_EQ( json.size(), 1u );
		EXPECT_EQ( json[0].size(), text.size()+2 ); //the value plus its two quotes.
	}
}
