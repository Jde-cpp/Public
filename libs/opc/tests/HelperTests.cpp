//opcHelpers.h/UAString.h/LocalizedText.h - the conversion primitives every other wrapper is built out of.  Get these
//wrong and the identifier round trips in NodeIdTests/ExNodeIdTests fail for reasons that have nothing to do with node ids.
#include <gtest/gtest.h>
#include <jde/opc/uatypes/NodeId.h>
#include <jde/opc/uatypes/UAString.h>

#define let const auto

namespace Jde::Opc::Tests{
	//data1/2/3 are native-endian in UA_Guid and big-endian (RFC 4122) in boost's uuid, so the conversion is a byte
	//shuffle, not a memcpy.  Pinned against a literal guid rather than round-tripped so a symmetric swap can't hide.
	constexpr UA_Guid testGuid{ 0x12345678, 0x1234, 0x5678, {0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78} };

	TEST( OpcHelperTests, StringConversions ){
		constexpr sv text{ "abc" };
		let ua = ToUV( text );
		EXPECT_EQ( ua.length, 3u );
		EXPECT_EQ( ToSV(ua), "abc" );
		EXPECT_EQ( ToString(ua), "abc" );
		EXPECT_EQ( (const void*)ua.data, (const void*)text.data() ); //ToUV borrows, it does not copy.

		auto alloc = AllocUAString( text );
		EXPECT_EQ( ToSV(alloc), "abc" );
		EXPECT_NE( (const void*)alloc.data, (const void*)text.data() ); //AllocUAString copies.
		UA_String_clear( &alloc );

		EXPECT_EQ( ToSV("abc"_uv), "abc" );
	}

	TEST( OpcHelperTests, UAStringOwnsItsBuffer ){
		UAString sized{ 8u };
		EXPECT_EQ( sized.length, 8u );
		EXPECT_NE( sized.data, nullptr );

		UAString empty{ 0u };
		EXPECT_EQ( empty.length, 0u );
		EXPECT_EQ( empty.data, nullptr );
	}

	TEST( OpcHelperTests, GuidConversions ){
		let boostGuid = ToGuid( testGuid );
		EXPECT_EQ( Jde::ToString(boostGuid), "12345678-1234-5678-1234-567812345678" );
		EXPECT_EQ( boostGuid.data[0], 0x12 ); //data1 landed big-endian.
		EXPECT_EQ( boostGuid.data[3], 0x78 );

		let round = ToUAGuid( boostGuid );
		EXPECT_EQ( 0, ::memcmp(&testGuid, &round, sizeof(UA_Guid)) );
		EXPECT_EQ( ToJson(testGuid), "12345678-1234-5678-1234-567812345678" );
		EXPECT_EQ( ToBinaryString(testGuid).size(), sizeof(UA_Guid) );
	}

	//#18 (fixed):  ToGuid used to std::erase the dashes and then hand the result to boost::lexical_cast<uuid>, but boost
	//1.87 rewrote uuid's operator>> around detail::from_chars, which *requires* the canonical dashed form - so the
	//de-dashed string parsed under no boost version and every guid-identified node id threw "Could not parse guid".
	//It now delegates to Jde::ToUuid, i.e. boost::uuids::string_generator, which detects the dashes and takes both.
	TEST( OpcHelperTests, ParseGuid ){
		UA_Guid dashed{};
		ASSERT_NO_THROW( ToGuid("12345678-1234-5678-1234-567812345678", dashed) ); //the form the serializer emits.
		EXPECT_EQ( 0, ::memcmp(&testGuid, &dashed, sizeof(UA_Guid)) );

		UA_Guid undashed{};
		ASSERT_NO_THROW( ToGuid("12345678123456781234567812345678", undashed) );
		EXPECT_EQ( 0, ::memcmp(&testGuid, &undashed, sizeof(UA_Guid)) );

		UA_Guid unused{};
		EXPECT_THROW( ToGuid("not-a-guid", unused), Exception );
	}

	//The round trip the serializer half depends on: ToJson emits what ToGuid reads back.
	TEST( OpcHelperTests, GuidTextRoundTrip ){
		UA_Guid parsed{};
		ASSERT_NO_THROW( ToGuid(string{ToJson(testGuid)}, parsed) );
		EXPECT_EQ( 0, ::memcmp(&testGuid, &parsed, sizeof(UA_Guid)) );
	}

	TEST( OpcHelperTests, ByteStringConversions ){
		const vector<uint8_t> bytes{ 0xde, 0xad, 0xbe, 0xef };
		let ua = ToUAByteString( bytes );
		ASSERT_EQ( ua->length, 4u );
		EXPECT_EQ( FromByteString(*ua), bytes );
		//Two encodings on purpose, for two audiences: hex for ByteString *values*, which have no parser, and base64 for a
		//node id's "b", which has to survive FromJson (reviews/opc-review2.md #7).
		EXPECT_EQ( ByteStringToJson(*ua), "deadbeef" );
		EXPECT_EQ( ByteStringToBase64(*ua), "3q2+7w==" );
		EXPECT_EQ( ByteStringToBase64(*ToUAByteString(vector<uint8_t>{})), "" );

		let none = ToUAByteString( vector<uint8_t>{} );
		EXPECT_EQ( none->length, 0u );
		EXPECT_TRUE( FromByteString(*none).empty() );
	}

	//The protobufjs Long form the gateway's javascript clients expect:  {high,low,unsigned}, not a bare number.
	TEST( OpcHelperTests, SixtyFourBitIntegersUseTheLongForm ){
		let u = ToJson( (UA_UInt64)0x0000000100000002ull );
		EXPECT_EQ( u.at("high").to_number<uint32_t>(), 1u );
		EXPECT_EQ( u.at("low").to_number<uint32_t>(), 2u );
		EXPECT_TRUE( u.at("unsigned").as_bool() );
		EXPECT_FALSE( ToJson( (UA_Int64)5 ).at("unsigned").as_bool() );
	}

	TEST( OpcHelperTests, IterableSkipsAnEmptyRange ){
		int values[]{ 1, 2, 3 };
		Iterable<int> some{ values, 3u };
		EXPECT_EQ( std::distance(some.begin(), some.end()), 3 );
		EXPECT_EQ( *some.begin(), 1 );

		Iterable<int> none{ values, 0u };
		EXPECT_EQ( none.begin(), none.end() );
	}

	TEST( OpcHelperTests, FindDataTypeResolvesVendorTypeIds ){
		EXPECT_EQ( FindDataType( NodeId{UA_TYPES[UA_TYPES_INT32].typeId} ), &UA_TYPES[UA_TYPES_INT32] );
		EXPECT_EQ( FindDataType( NodeId{2, 5002} ), nullptr );
	}

	TEST( LocalizedTextTests, ToJson ){
		LocalizedText both{ "hello", "en" };
		let j = both.ToJson();
		EXPECT_EQ( j.at("text").as_string(), "hello" );
		EXPECT_EQ( j.at("locale").as_string(), "en" );

		LocalizedText textOnly{ "hello" };
		EXPECT_FALSE( textOnly.ToJson().contains("locale") ); //an empty locale is omitted, not serialized as "".

		EXPECT_TRUE( LocalizedText{}.ToJson().empty() );      //and an empty text drops the whole object.
	}

	//L3: UAString.h used to guard itself with `#ifndef UA_STRING`, but UA_STRING is open62541's own exported function
	//(types.h) and the expansion of UA_BYTESTRING.  Once that header had been seen, `UA_STRING(chars)` expanded to
	//`(chars)` inside any vendor header that followed.  It guards with #pragma once now.
#ifdef UA_STRING
	constexpr bool uaStringGuardShadowsTheVendorSymbol = true;
#else
	constexpr bool uaStringGuardShadowsTheVendorSymbol = false;
#endif
	TEST( OpcHelperTests, UAStringGuardDoesNotShadowTheVendorSymbol ){
		EXPECT_FALSE( uaStringGuardShadowsTheVendorSymbol );
	}
}
