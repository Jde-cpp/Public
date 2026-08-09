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

	//#3 (fixed): the ToJson fallback sat *inside* the per-element loop and re-encoded the whole variant each time, and the
	//IS() chain had no LOCALIZEDTEXT/QUALIFIEDNAME arm - a LocalizedText[3] serialized as [[a,b,c],[a,b,c],[a,b,c]].
	TEST( ValueTests, LocalizedTextArraySerializesPerElement ){
		UA_LocalizedText texts[3]{};
		for( uint i=0; i<3; ++i ){
			texts[i].locale = UA_STRING_NULL;
			texts[i].text = AllocUAString( string(1, (char)('a'+i)) );
		}
		let v = arrayDataValue( texts, 3u, UA_TYPES[UA_TYPES_LOCALIZEDTEXT] );
		for( auto& t : texts )
			UA_LocalizedText_clear( &t );
		EXPECT_EQ( serialize(v.ToJson()), R"(["a","b","c"])" );

		UA_LocalizedText one{};
		one.locale = UA_STRING_NULL;
		one.text = AllocUAString( string{"solo"} );
		let scalar = dataValue( &one, UA_TYPES[UA_TYPES_LOCALIZEDTEXT] );
		UA_LocalizedText_clear( &one );
		EXPECT_EQ( scalar.ToJson().as_string(), "solo" ); //scalar and array agree on the shape.
	}

	//The other arm #3 added.  Note the shape is BrowseName's {ns,name}, matching the scalar case in Variant::ToJson,
	//rather than the vendor's UA-json {Name,Uri}.
	TEST( ValueTests, QualifiedNameArraySerializesPerElement ){
		UA_QualifiedName names[2]{};
		names[0] = UA_QualifiedName{ 2, AllocUAString(string{"First"}) };
		names[1] = UA_QualifiedName{ 3, AllocUAString(string{"Second"}) };
		let v = arrayDataValue( names, 2u, UA_TYPES[UA_TYPES_QUALIFIEDNAME] );
		for( auto& n : names )
			UA_QualifiedName_clear( &n );
		let j = v.ToJson();
		ASSERT_EQ( j.as_array().size(), 2u );
		EXPECT_EQ( j.as_array()[0].at("name").as_string(), "First" );
		EXPECT_EQ( j.as_array()[0].at("ns").to_number<int>(), 2 );
		EXPECT_EQ( j.as_array()[1].at("name").as_string(), "Second" );
	}

	//The #3/#11 composite, and the one input that used to take the gateway down.  A nested variant has no IS() arm, so it
	//goes through the ε fallback encoder; at 4 KB it exceeded #11's 2096-byte cap, and the throw crossed ToJson's Ι
	//boundary - std::terminate, driven purely by what the OPC server publishes.  #11 removed the cap so it now encodes,
	//and #3's catch stays as defence for any other encoder failure.  If the Ι boundary regresses the whole runner aborts
	//rather than one test failing: a noexcept violation is not catchable.
	TEST( ValueTests, LargeNestedVariantEncodesThroughToJson ){
		let big = string( 4096, 'x' );
		let uaBig = ToUV( big );
		UA_Variant inner{};
		UA_Variant_setScalarCopy( &inner, &uaBig, &UA_TYPES[UA_TYPES_STRING] );
		let v = dataValue( &inner, UA_TYPES[UA_TYPES_VARIANT] );
		UA_Variant_clear( &inner );

		let j = v.ToJson();
		ASSERT_FALSE( j.is_null() );
		EXPECT_GT( serialize(j).size(), big.size() ) << "the payload has to survive the encode, not be dropped or capped";
	}

	//#13 (fixed): AsNumber<T> round-tripped through ToJson, which returns {high,low,unsigned} for the 64-bit integers, and
	//Json::AsNumber then threw on an object.  SoakRunner and SubscribeTests both call it, and the Int64/UInt64 legs only
	//WARN, so a healthy server read as lost round trips.  It reads the scalar straight out of value.data now.
	TEST( ValueTests, AsNumberReadsSixtyFourBitIntegers ){
		const UA_Int64 i64{ 42 };
		EXPECT_EQ( dataValue(&i64, UA_TYPES[UA_TYPES_INT64]).AsNumber<_int>(), 42 );

		const UA_UInt64 u64{ 42 };
		EXPECT_EQ( dataValue(&u64, UA_TYPES[UA_TYPES_UINT64]).AsNumber<uint>(), 42u );

		//past 2^53, where a detour through a double would start rounding.
		const UA_Int64 big{ 9'007'199'254'740'993 };
		EXPECT_EQ( dataValue(&big, UA_TYPES[UA_TYPES_INT64]).AsNumber<_int>(), 9'007'199'254'740'993 );
		const UA_UInt64 huge{ 18'446'744'073'709'551'615ull };
		EXPECT_EQ( dataValue(&huge, UA_TYPES[UA_TYPES_UINT64]).AsNumber<uint>(), 18'446'744'073'709'551'615ull );
		const UA_Int64 negative{ -9'007'199'254'740'993 };
		EXPECT_EQ( dataValue(&negative, UA_TYPES[UA_TYPES_INT64]).AsNumber<_int>(), -9'007'199'254'740'993 );
	}

	//Every numeric scalar, since a direct read supports a different set than the json detour did.
	TEST( ValueTests, AsNumberCoversTheNumericScalars ){
		const UA_Boolean b{ true };
		EXPECT_EQ( dataValue(&b, UA_TYPES[UA_TYPES_BOOLEAN]).AsNumber<int>(), 1 );
		const UA_SByte sbyte{ -3 };
		EXPECT_EQ( dataValue(&sbyte, UA_TYPES[UA_TYPES_SBYTE]).AsNumber<int>(), -3 );
		const UA_Byte byte{ 200 };
		EXPECT_EQ( dataValue(&byte, UA_TYPES[UA_TYPES_BYTE]).AsNumber<int>(), 200 );
		const UA_Int16 i16{ -300 };
		EXPECT_EQ( dataValue(&i16, UA_TYPES[UA_TYPES_INT16]).AsNumber<int>(), -300 );
		const UA_UInt32 u32{ 42 };
		EXPECT_EQ( dataValue(&u32, UA_TYPES[UA_TYPES_UINT32]).AsNumber<uint>(), 42u );
		const UA_Float f{ 1.5f };
		EXPECT_DOUBLE_EQ( dataValue(&f, UA_TYPES[UA_TYPES_FLOAT]).AsNumber<double>(), 1.5 );
		const UA_StatusCode sc{ UA_STATUSCODE_GOODCLAMPED };
		EXPECT_EQ( dataValue(&sc, UA_TYPES[UA_TYPES_STATUSCODE]).AsNumber<uint32_t>(), UA_STATUSCODE_GOODCLAMPED );

		let text = ToUV( "abc" );
		EXPECT_THROW( dataValue(&text, UA_TYPES[UA_TYPES_STRING]).AsNumber<int>(), Exception ); //not a number, and says so.
	}

	//The json path rejected a value too wide for T; a direct read must not quietly truncate instead.
	TEST( ValueTests, AsNumberRejectsAValueTooWideForTheTarget ){
		const UA_Int64 big{ 1'000'000 };
		EXPECT_THROW( dataValue(&big, UA_TYPES[UA_TYPES_INT64]).AsNumber<int16_t>(), Exception );
		const UA_Int32 negative{ -1 };
		EXPECT_THROW( dataValue(&negative, UA_TYPES[UA_TYPES_INT32]).AsNumber<uint>(), Exception );
		EXPECT_EQ( dataValue(&big, UA_TYPES[UA_TYPES_INT64]).AsNumber<int32_t>(), 1'000'000 ); //fits, so no throw.
	}

	//An empty or status-only value used to report the misleading "Arrays Not implemented."
	TEST( ValueTests, AsNumberOnAnEmptyValueNamesTheStatus ){
		Value bad{ (StatusCode)UA_STATUSCODE_BADNODEIDUNKNOWN };
		EXPECT_THROW( bad.AsNumber<uint>(), Exception );
		try{ bad.AsNumber<uint>(); }
		catch( const Exception& e ){ EXPECT_NE( string{e.what()}.find("80340000"), string::npos ) << e.what(); }

		const UA_Int32 values[]{ 1, 2 };
		EXPECT_THROW( arrayDataValue(values, 2u, UA_TYPES[UA_TYPES_INT32]).AsNumber<int>(), Exception );
	}

	Ω statusValue( StatusCode sc, const UA_Double* d )ι->Value{
		UA_DataValue dv{};
		if( d ){
			UA_Variant_setScalarCopy( &dv.value, d, &UA_TYPES[UA_TYPES_DOUBLE] );
			dv.hasValue = true;
		}
		dv.hasStatus = true;
		dv.status = sc;
		return Value{ move(dv) };
	}

	//#15 (fixed): any non-zero StatusCode replaced the reading, but an Uncertain code (0x4xxxxxxx) carries a usable value
	//per OPC-UA - a sensor reporting UNCERTAINLASTUSABLEVALUE with 42.7 showed up as a blank tag.  Now a non-zero code
	//rides along with the reading - {"v":…,"sc":…} - rather than replacing it; only Bad still does.
	TEST( ValueTests, UncertainQualityKeepsTheValue ){
		const UA_Double d{ 42.7 };
		EXPECT_DOUBLE_EQ( statusValue(UA_STATUSCODE_GOOD, &d).ToJson().to_number<double>(), 42.7 );//plain Good stays a bare scalar.

		//Uncertain codes and an informative Good sub-code all report both halves.
		for( let sc : { UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE, UA_STATUSCODE_UNCERTAINSENSORNOTACCURATE,
				UA_STATUSCODE_UNCERTAIN, UA_STATUSCODE_GOODCLAMPED } ){
			let j = statusValue( sc, &d ).ToJson();
			EXPECT_DOUBLE_EQ( j.at("v").to_number<double>(), 42.7 ) << Ƒ( "0x{:x}", sc );
			EXPECT_EQ( j.at("sc").to_number<uint32_t>(), sc ) << Ƒ( "0x{:x}", sc );
		}

		//...but a Bad one still replaces it: there is no usable reading behind a Bad status.
		for( let sc : { UA_STATUSCODE_BADNODEIDUNKNOWN, UA_STATUSCODE_BADTIMEOUT } ){
			let j = statusValue( sc, &d ).ToJson();
			EXPECT_EQ( j.at("sc").to_number<uint32_t>(), sc ) << Ƒ( "0x{:x}", sc );
			EXPECT_FALSE( j.get_object().contains("v") ) << Ƒ( "0x{:x}", sc );
		}

		//An Uncertain array wraps the same way: the whole array under "v".
		const UA_Int32 values[]{ 1, 2 };
		auto arr = arrayDataValue( values, 2u, UA_TYPES[UA_TYPES_INT32] );
		arr.status = UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE; arr.hasStatus = true;
		let ja = arr.ToJson();
		EXPECT_EQ( ja.at("sc").to_number<uint32_t>(), UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE );
		ASSERT_TRUE( ja.at("v").is_array() );
		EXPECT_EQ( ja.at("v").as_array().size(), 2u );
		EXPECT_EQ( ja.at("v").as_array()[1].to_number<int>(), 2 );
	}

	//With nothing to show, the status is still worth reporting - otherwise an Uncertain-but-empty value became a bare null.
	TEST( ValueTests, AnEmptyValueStillReportsItsStatus ){
		EXPECT_EQ( statusValue(UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE, nullptr).ToJson().at("sc").to_number<uint32_t>(),
			UA_STATUSCODE_UNCERTAINLASTUSABLEVALUE );
		EXPECT_TRUE( Value( UA_DataValue{} ).ToJson().is_null() ); //no status, no value: still just null.
	}

	//#20: Get<T> reinterprets value.data, it does not convert, so it has to be handed the raw UA type and the wrapper
	//constructed from that.  Asking Get for a UADateTime read a tick count as a TimePoint instead: a 2001 date came back
	//as 12644473600s - off by exactly the 1601->1970 offset, 369 years - and a 1500ms Duration as 465431188521 seconds.
	TEST( ValueTests, GetReturnsTheRawUaTypeNotAWrapper ){
		const UA_DateTime dt = UA_DateTime_fromUnixTime( 1'000'000'000 );
		let v = dataValue( &dt, UA_TYPES[UA_TYPES_DATETIME] );
		EXPECT_EQ( v.Get<UA_DateTime>(0), dt );
		EXPECT_EQ( UADateTime{v.Get<UA_DateTime>(0)}.ToProto().seconds(), 1'000'000'000 );

		const UA_Duration ms{ 1500 };//a millisecond double, not a time point - there is no wrapper to ask for at all.
		EXPECT_DOUBLE_EQ( dataValue(&ms, UA_TYPES[UA_TYPES_DURATION]).Get<UA_Duration>(0), 1500. );
	}

	//L1: ToJson emits a Duration as a plain millisecond number while Set demanded a {seconds,nanos} object, so writing
	//back a value that was just read threw in as_object().
	TEST( ValueTests, DurationRoundTrip ){
		const UA_Duration ms{ 1500 };
		let read = dataValue( &ms, UA_TYPES[UA_TYPES_DURATION] ).ToJson();
		EXPECT_DOUBLE_EQ( read.to_number<double>(), 1500. );//a bare number, which is what has to come back.
		auto write = dataValue( &ms, UA_TYPES[UA_TYPES_DURATION] );
		ASSERT_NO_THROW( write.Set(read) );
		EXPECT_DOUBLE_EQ( write.ToJson().to_number<double>(), 1500. );
		EXPECT_EQ( write.value.type, &UA_TYPES[UA_TYPES_DURATION] );//still a Duration, not demoted to a plain Double.

		//The object form has to keep working: the socket sends a Duration as a google.protobuf.Duration, so a client can
		//be writing back {seconds,nanos} for the very same node.
		auto fromObject = dataValue( &ms, UA_TYPES[UA_TYPES_DURATION] );
		ASSERT_NO_THROW( fromObject.Set(jvalue{jobject{ {"seconds",2}, {"nanos",500'000'000} }}) );
		EXPECT_DOUBLE_EQ( fromObject.ToJson().to_number<double>(), 2500. );

		//Double takes both spellings too - it always did, and merging the branches must not have cost it either.
		const UA_Double d{ 1500 };
		auto dbl = dataValue( &d, UA_TYPES[UA_TYPES_DOUBLE] );
		ASSERT_NO_THROW( dbl.Set(jvalue{1500.}) );
		EXPECT_DOUBLE_EQ( dbl.ToJson().to_number<double>(), 1500. );
		ASSERT_NO_THROW( dbl.Set(jvalue{jobject{ {"seconds",1}, {"nanos",500'000'000} }}) );
		EXPECT_DOUBLE_EQ( dbl.ToJson().to_number<double>(), 1500. );
	}

	//L2: the old `seconds + nanos` added two chrono durations whose common type is int64 *nanoseconds*, so scaling the
	//seconds up overflowed past ~292 years.  Signed overflow, so UB - a UBSan abort where it is enabled, and a silently
	//wrapped value everywhere else, from a number a client picked.
	TEST( ValueTests, DurationSecondsPastTheNanosecondRange ){
		const UA_Duration ms{ 0 };
		auto v = dataValue( &ms, UA_TYPES[UA_TYPES_DURATION] );
		for( let seconds : { 100'000'000'000LL, -100'000'000'000LL } ){//~3170 years, an order past where it used to wrap.
			ASSERT_NO_THROW( v.Set(jvalue{jobject{ {"seconds", seconds}, {"nanos", 0} }}) ) << seconds;
			EXPECT_DOUBLE_EQ( v.ToJson().to_number<double>(), (double)seconds*1000. ) << seconds;
		}
	}

	//Below the cut (fixed): the typeless Set deduced null/int64/uint64/double/string but not bool, so writing `true` to a
	//node whose DataType had not been resolved failed with "Value has no type."  Fixed as part of #8 rather than on its
	//own: Variant( jvalue, dataType ) now delegates here, so leaving the gap would have regressed a `{value:true}` config
	//that used to work through Variant's own inference.
	TEST( ValueTests, TypelessSetInfersBool ){
		Value v{ UA_DataValue{} };
		ASSERT_NO_THROW( v.Set(jvalue{true}) );
		EXPECT_TRUE( v.ToJson().as_bool() );
		EXPECT_EQ( v.value.type, &UA_TYPES[UA_TYPES_BOOLEAN] );
	}
}
