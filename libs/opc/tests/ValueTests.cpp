//Value is the read path (UA_DataValue -> json for the gateway's clients) and the write path (json -> UA_DataValue).
//Whatever ToJson emits for a type, Set has to accept back.
#include <gtest/gtest.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/Value.h>

#define let const auto

namespace Jde::Opc::Tests{
	Ω dataValue( const void* value, const UA_DataType& type )ι->Value{
		UA_DataValue dv{};
		UA_Variant_setScalarCopy( &dv.value, value, &type );
		dv.hasValue = true;
		return Value{ move(dv) };
	}
	Ω arrayDataValue( const void* values, uint count, const UA_DataType& type )ι->Value{
		UA_DataValue dv{};
		UA_Variant_setArrayCopy( &dv.value, values, count, &type );
		dv.hasValue = true;
		return Value{ move(dv) };
	}

	TEST( ValueTests, EmptyAndStatus ){
		EXPECT_TRUE( Value( UA_DataValue{} ).IsEmpty() );
		EXPECT_TRUE( Value( UA_DataValue{} ).ToJson().is_null() );

		Value bad{ (StatusCode)UA_STATUSCODE_BADNODEIDUNKNOWN };
		let j = bad.ToJson();
		EXPECT_EQ( j.at("sc").to_number<uint32_t>(), UA_STATUSCODE_BADNODEIDUNKNOWN );
	}

	TEST( ValueTests, ScalarToJson ){
		const UA_Boolean b{ true };
		EXPECT_TRUE( dataValue(&b, UA_TYPES[UA_TYPES_BOOLEAN]).ToJson().as_bool() );

		const UA_Int16 i16{ -3 };
		EXPECT_EQ( dataValue(&i16, UA_TYPES[UA_TYPES_INT16]).ToJson().to_number<int>(), -3 );

		const UA_Int32 i32{ 42 };
		EXPECT_EQ( dataValue(&i32, UA_TYPES[UA_TYPES_INT32]).ToJson().to_number<int>(), 42 );

		const UA_UInt32 u32{ 42 };
		EXPECT_EQ( dataValue(&u32, UA_TYPES[UA_TYPES_UINT32]).ToJson().to_number<uint32_t>(), 42u );

		const UA_Double d{ 1.5 };
		EXPECT_DOUBLE_EQ( dataValue(&d, UA_TYPES[UA_TYPES_DOUBLE]).ToJson().to_number<double>(), 1.5 );

		const UA_Float f{ 1.5f };
		EXPECT_DOUBLE_EQ( dataValue(&f, UA_TYPES[UA_TYPES_FLOAT]).ToJson().to_number<double>(), 1.5 );

		const UA_Byte byte{ 200 };
		EXPECT_EQ( dataValue(&byte, UA_TYPES[UA_TYPES_BYTE]).ToJson().to_number<int>(), 200 );

		const UA_SByte sbyte{ -100 };
		EXPECT_EQ( dataValue(&sbyte, UA_TYPES[UA_TYPES_SBYTE]).ToJson().to_number<int>(), -100 );

		let text = ToUV( "abc" );
		EXPECT_EQ( dataValue(&text, UA_TYPES[UA_TYPES_STRING]).ToJson().as_string(), "abc" );

		const UA_StatusCode sc{ UA_STATUSCODE_GOODCLAMPED };
		EXPECT_EQ( dataValue(&sc, UA_TYPES[UA_TYPES_STATUSCODE]).ToJson().to_number<uint32_t>(), UA_STATUSCODE_GOODCLAMPED );
	}

	//The 64-bit integers use the protobufjs Long form so javascript clients keep full precision.
	TEST( ValueTests, SixtyFourBitIntegersToJson ){
		const UA_Int64 i64{ 0x0000000100000002ll };
		let j = dataValue( &i64, UA_TYPES[UA_TYPES_INT64] ).ToJson();
		EXPECT_EQ( j.at("high").to_number<uint32_t>(), 1u );
		EXPECT_EQ( j.at("low").to_number<uint32_t>(), 2u );
		EXPECT_FALSE( j.at("unsigned").as_bool() );

		const UA_UInt64 u64{ 5 };
		EXPECT_TRUE( dataValue(&u64, UA_TYPES[UA_TYPES_UINT64]).ToJson().at("unsigned").as_bool() );
	}

	TEST( ValueTests, NodeIdAndGuidToJson ){
		let n = NodeId{ 2, 5002 };
		let j = dataValue( static_cast<const UA_NodeId*>(&n), UA_TYPES[UA_TYPES_NODEID] ).ToJson();
		EXPECT_EQ( j.at("ns").to_number<int>(), 2 );
		EXPECT_EQ( j.at("i").to_number<int>(), 5002 );

		constexpr UA_Guid guid{ 0x12345678, 0x1234, 0x5678, {0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78} };
		EXPECT_EQ( dataValue(&guid, UA_TYPES[UA_TYPES_GUID]).ToJson().as_string(), "12345678-1234-5678-1234-567812345678" );
	}

	TEST( ValueTests, ArrayToJson ){
		const UA_Int32 values[]{ 1, 2, 3 };
		EXPECT_EQ( serialize(arrayDataValue(values, 3u, UA_TYPES[UA_TYPES_INT32]).ToJson()), "[1,2,3]" );

		const UA_Double doubles[]{ 1.5, 2.5 };
		let asJson = arrayDataValue( doubles, 2u, UA_TYPES[UA_TYPES_DOUBLE] ).ToJson();
		ASSERT_EQ( asJson.as_array().size(), 2u );
		EXPECT_DOUBLE_EQ( asJson.as_array()[0].to_number<double>(), 1.5 );
		EXPECT_DOUBLE_EQ( asJson.as_array()[1].to_number<double>(), 2.5 );
	}

	//Set -> ToJson has to be the identity for every type the write path claims to support.
	TEST( ValueTests, SetRoundTrip ){
		EXPECT_TRUE( Value( jvalue{true}, &UA_TYPES[UA_TYPES_BOOLEAN] ).ToJson().as_bool() );
		EXPECT_EQ( Value( jvalue{42}, &UA_TYPES[UA_TYPES_INT16] ).ToJson().to_number<int>(), 42 );
		EXPECT_EQ( Value( jvalue{42}, &UA_TYPES[UA_TYPES_INT32] ).ToJson().to_number<int>(), 42 );
		EXPECT_EQ( Value( jvalue{42}, &UA_TYPES[UA_TYPES_UINT32] ).ToJson().to_number<uint32_t>(), 42u );
		EXPECT_EQ( Value( jvalue{200}, &UA_TYPES[UA_TYPES_BYTE] ).ToJson().to_number<int>(), 200 );
		EXPECT_DOUBLE_EQ( Value( jvalue{1.5}, &UA_TYPES[UA_TYPES_DOUBLE] ).ToJson().to_number<double>(), 1.5 );
		EXPECT_DOUBLE_EQ( Value( jvalue{1.5}, &UA_TYPES[UA_TYPES_FLOAT] ).ToJson().to_number<double>(), 1.5 );
		EXPECT_EQ( Value( jvalue{"abc"}, &UA_TYPES[UA_TYPES_STRING] ).ToJson().as_string(), "abc" );
	}

	TEST( ValueTests, SetRejectsTheWrongJsonKind ){
		EXPECT_THROW( Value( jvalue{42}, &UA_TYPES[UA_TYPES_BOOLEAN] ), Exception );
		EXPECT_THROW( Value( jvalue{42}, &UA_TYPES[UA_TYPES_STRING] ), Exception );
		EXPECT_THROW( Value( jvalue{nullptr}, nullptr ), Exception );                     //no type and no value to infer from.
		EXPECT_THROW( Value( jvalue{5}, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT] ), Exception ); //not implemented, and says so.
	}

	//A typeless Value infers the UA type from the json kind - the path a write takes before the node's DataType is known.
	TEST( ValueTests, TypelessSetInfers ){
		EXPECT_FALSE( Value( jvalue{5}, nullptr ).ToJson().at("unsigned").as_bool() ); //int64 -> the Long form.
		EXPECT_EQ( Value( jvalue{"abc"}, nullptr ).ToJson().as_string(), "abc" );
		EXPECT_DOUBLE_EQ( Value( jvalue{1.5}, nullptr ).ToJson().to_number<double>(), 1.5 );
	}

	TEST( ValueTests, AsNumberOnThirtyTwoBitScalars ){
		const UA_Int32 i32{ 42 };
		EXPECT_EQ( dataValue(&i32, UA_TYPES[UA_TYPES_INT32]).AsNumber<int32_t>(), 42 );

		const UA_Double d{ 42.5 };
		EXPECT_DOUBLE_EQ( dataValue(&d, UA_TYPES[UA_TYPES_DOUBLE]).AsNumber<double>(), 42.5 );

		const UA_Int32 values[]{ 1, 2 };
		EXPECT_THROW( arrayDataValue(values, 2u, UA_TYPES[UA_TYPES_INT32]).AsNumber<int32_t>(), Exception );
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#3: the ToJson fallback sits *inside* the per-element loop and re-encodes the whole variant each time, and the IS()
	//chain has no LOCALIZEDTEXT/QUALIFIEDNAME arm - a LocalizedText[3] serializes as [[a,b,c],[a,b,c],[a,b,c]].  (The
	//other half of #3 is that the fallback throws from a Ι function, which terminates the gateway; that one cannot be
	//expressed as an assertion.)
	TEST( ValueTests, DISABLED_R2_3_UnhandledArrayTypeSerializesPerElement ){
		UA_LocalizedText texts[3]{};
		for( uint i=0; i<3; ++i ){
			texts[i].locale = UA_STRING_NULL;
			texts[i].text = AllocUAString( string(1, (char)('a'+i)) );
		}
		let v = arrayDataValue( texts, 3u, UA_TYPES[UA_TYPES_LOCALIZEDTEXT] );
		for( auto& t : texts )
			UA_LocalizedText_clear( &t );
		EXPECT_EQ( serialize(v.ToJson()), R"(["a","b","c"])" );
	}

	//#13: AsNumber<T> round-trips through ToJson, which returns {high,low,unsigned} for the 64-bit integers, and
	//Json::AsNumber then throws on an object.  SoakRunner and SubscribeTests both call it, and the Int64/UInt64 legs only
	//WARN, so a healthy server reads as lost round trips.  Read the scalar straight out of value.data instead.
	TEST( ValueTests, DISABLED_R2_13_AsNumberReadsSixtyFourBitIntegers ){
		const UA_Int64 i64{ 42 };
		EXPECT_EQ( dataValue(&i64, UA_TYPES[UA_TYPES_INT64]).AsNumber<_int>(), 42 );

		const UA_UInt64 u64{ 42 };
		EXPECT_EQ( dataValue(&u64, UA_TYPES[UA_TYPES_UINT64]).AsNumber<uint>(), 42u );
	}

	//#15: any non-zero StatusCode replaces the reading, but an Uncertain code (0x4xxxxxxx) carries a usable value per
	//OPC-UA - a sensor reporting UNCERTAINLASTUSABLEVALUE with 42.7 shows up as a blank tag.  Gate on severity.
	TEST( ValueTests, DISABLED_R2_15_UncertainQualityKeepsTheValue ){
		UA_DataValue dv{};
		const UA_Double d{ 42.7 };
		UA_Variant_setScalarCopy( &dv.value, &d, &UA_TYPES[UA_TYPES_DOUBLE] );
		dv.hasValue = true;
		dv.hasStatus = true;
		dv.status = UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE;
		let v = Value{ move(dv) };
		EXPECT_DOUBLE_EQ( v.ToJson().to_number<double>(), 42.7 );
	}

	//Below the cut: ToJson emits a Duration as a plain millisecond number while Set demands a {seconds,nanos} object, so
	//writing back a value that was just read throws in as_object().
	TEST( ValueTests, DISABLED_R2_DurationRoundTrip ){
		const UA_Duration ms{ 1500 };
		let read = dataValue( &ms, UA_TYPES[UA_TYPES_DURATION] ).ToJson();
		auto write = dataValue( &ms, UA_TYPES[UA_TYPES_DURATION] );
		ASSERT_NO_THROW( write.Set(read) );
		EXPECT_DOUBLE_EQ( write.ToJson().to_number<double>(), 1500. );
	}

	//Below the cut: the typeless Set deduces null/int64/uint64/double/string but not bool, so writing `true` to a node
	//whose DataType has not been resolved fails with "Value has no type."
	TEST( ValueTests, DISABLED_R2_TypelessSetInfersBool ){
		Value v{ UA_DataValue{} };
		ASSERT_NO_THROW( v.Set(jvalue{true}) );
		EXPECT_TRUE( v.ToJson().as_bool() );
	}
}
