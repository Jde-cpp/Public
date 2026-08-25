//Variant is the persistence path:  ToUAJson() writes one UA-JSON string per element into the db, ToUAValues() reads
//them back, ToArrayDims()/ArrayDimString() carry the shape alongside.  These are the round trips reviews/opc-review2.md
//says would have caught most of the review.
#include <gtest/gtest.h>
#include <jde/db/Value.h>
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

	//review2 #3's note, closed as O4: the NODEID/QUALIFIEDNAME arms lived in ToJson's *scalar* branch only, so one type
	//gave two shapes depending on its value rank - {ns,i} on its own, the vendor's UA-json spelling inside an array.
	//The arms moved into ElementToJson, which both ranks go through.  The UtcTime case is the other half: dispatch was on
	//the descriptor address, so an alias took the vendor spelling here while Value::ToJson gave it {seconds,nanos}.
	TEST( VariantTests, ArrayElementsUseTheSameShapeAsAScalar ){
		const UA_NodeId ids[]{ {2, UA_NODEIDTYPE_NUMERIC, {5002}}, {3, UA_NODEIDTYPE_NUMERIC, {5003}} };
		let arrayJson = Variant{ arrayVariant(ids, 2u, UA_TYPES[UA_TYPES_NODEID]) }.ToJson( true );
		ASSERT_TRUE( arrayJson.is_array() );
		EXPECT_EQ( arrayJson.as_array()[1].at("ns").to_number<int>(), 3 );
		EXPECT_EQ( arrayJson.as_array()[1].at("i").to_number<int>(), 5003 );

		let scalarJson = Variant{ scalarVariant(&ids[1], UA_TYPES[UA_TYPES_NODEID]) }.ToJson( true );
		EXPECT_EQ( serialize(arrayJson.as_array()[1]), serialize(scalarJson) ) << "one type, one shape, whatever the value rank";

		//UtcTime is a DateTime by kind, so it takes the DateTime arm rather than the vendor's spelling.
		const UA_DateTime utc{ UA_DateTime_fromUnixTime(1700000000) };
		let utcJson = Variant{ scalarVariant(&utc, UA_TYPES[UA_TYPES_UTCTIME]) }.ToJson( true );
		EXPECT_EQ( utcJson.at("seconds").to_number<int64_t>(), 1700000000 );
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

	//review3 #3: ArrayDimString() returns "" for a scalar and VariantInsertAwait bound that string, which sqlite, mysql
	//and odbc all store as a *non-null* empty text - so VariantAwait's `isArray`, derived from the column, was true for
	//every row and every persisted scalar reloaded as a one-element array.  UA_Server_writeValue then answers
	//BadTypeMismatch for array data on a scalar-ranked member, i.e. every object instantiated after a restart failed.
	//ArrayDimValue() owns the shape instead.  This walks the caller's contract in VariantAwait's order: bind the value,
	//derive isArray from what came back, hand that to ToUAValues.
	TEST( VariantTests, AScalarPersistsNullDimensions ){
		const UA_Int32 i{ 5 };
		Variant scalar{ scalarVariant(&i, UA_TYPES[UA_TYPES_INT32]) };
		ASSERT_TRUE( scalar.IsScalar() );
		EXPECT_TRUE( scalar.ArrayDimString().empty() );
		EXPECT_FALSE( DB::Value{string{}}.is_null() ) << "the old binding: an empty dims string is a value, not NULL";
		let dims = scalar.ArrayDimValue();
		ASSERT_TRUE( dims.is_null() ) << "a non-null column is what made the reload call this an array";

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], stored(scalar.ToUAJson()), !dims.is_null() );
		ASSERT_NE( data, nullptr );
		EXPECT_EQ( count, 0u );
		Variant round{ 0, {count, data}, Variant::ToArrayDims(""), UA_TYPES[UA_TYPES_INT32] };
		EXPECT_TRUE( round.IsScalar() );
		EXPECT_EQ( serialize(round.ToJson(true)), "5" );
	}

	//The other half, and review2 #10's residual: an array that declares no arrayDimensions - what UA_Variant_setArrayCopy
	//leaves - would reload as a scalar if the column were NULL for it too, since one element cannot be told from a scalar
	//by its count.  It persists its length instead, so the same walk keeps it an array.
	TEST( VariantTests, AnArrayWithNoDimensionsPersistsItsLength ){
		const UA_Int32 one[]{ 7 };
		Variant array{ arrayVariant(one, 1, UA_TYPES[UA_TYPES_INT32]) };
		ASSERT_FALSE( array.IsScalar() );
		EXPECT_TRUE( array.ArrayDimString().empty() ) << "setArrayCopy sets arrayLength, not arrayDimensions";
		let dims = array.ArrayDimValue();
		ASSERT_FALSE( dims.is_null() );
		EXPECT_EQ( dims.get_string(), "1" );

		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], stored(array.ToUAJson()), !dims.is_null() );
		ASSERT_NE( data, nullptr );
		EXPECT_EQ( count, 1u );
		Variant round{ 0, {count, data}, Variant::ToArrayDims(dims.get_string()), UA_TYPES[UA_TYPES_INT32] };
		EXPECT_FALSE( round.IsScalar() );
		EXPECT_EQ( serialize(round.ToJson(true)), "[7]" );
		EXPECT_EQ( round.ArrayDimValue().get_string(), "1" ) << "write->read->write has to be stable";
	}

	//review3 #12: ToUAValues documents "the variant loads as null" and returns {0,nullptr} for a row it cannot decode,
	//but the receiving ctor stamped `type` and the caller's arrayDimensions onto it anyway.  The result was a *typed*
	//variant with data==NULL - UA_Variant_isEmpty looks at `type` alone, so IsNull() was false and ToJson() gave [] -
	//and with stored dims the binary encoder answers BadEncodingError, which closes the channel on every Read that
	//touches the node.  Logs one expected ERR, the same one ToUAValuesNeedsRawUaJson above provokes.
	TEST( VariantTests, ADecodeFailureLoadsAsAGenuinelyNullVariant ){
		flat_map<uint,string> quoted;
		quoted.emplace( 0u, serialize(jvalue{"5"}) );//json-quoted: the pre-review2 column shape ToUAValues' comment names.
		let [count, data] = Variant::ToUAValues( UA_TYPES[UA_TYPES_INT32], move(quoted), false );
		ASSERT_EQ( data, nullptr );

		Variant v{ 7, {count, data}, Variant::ToArrayDims("2,3"), UA_TYPES[UA_TYPES_INT32] };//dims the ctor now has to free.
		EXPECT_TRUE( v.IsNull() );
		EXPECT_EQ( v.type, nullptr );
		EXPECT_EQ( v.arrayLength, 0u );
		EXPECT_EQ( v.arrayDimensionsSize, 0u );
		EXPECT_EQ( v.arrayDimensions, nullptr );
		EXPECT_TRUE( v.ToUAJson().empty() );
		EXPECT_EQ( v.VariantPK, 7u );//the pk still identifies the row that could not be read.
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
