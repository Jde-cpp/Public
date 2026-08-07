//NodeId: ownership, ordering, and the three round trips the review asks for - ToJson->FromJson, ToString->DecodeJson
//and InsertParams (the shape the insert procs dispatch on) - across all four identifier kinds.
#include <concepts>
#include <gtest/gtest.h>
#include <jde/db/Value.h>
#include <jde/opc/uatypes/NodeId.h>

#define let const auto

namespace Jde::Opc::Tests{
	Ω numeric( UA_UInt16 ns, UA_UInt32 id )ι->NodeId{ return NodeId{ ns, id }; }
	Ω fromJson( sv json )ε->NodeId{ return NodeId{ parse(json) }; }
	//The wrapper's own ctors cannot build a guid or byte-string id from json today (see DISABLED_ParseGuidAcceptsTheCanonicalForm
	//and DISABLED_R2_7_ByteStringJsonRoundTrip), so these two assemble the raw base and hand ownership over.
	Ω guidNode( UA_UInt16 ns, const UA_Guid& guid )ι->NodeId{
		UA_NodeId ua{};
		ua.namespaceIndex = ns;
		ua.identifierType = UA_NODEIDTYPE_GUID;
		ua.identifier.guid = guid;
		return NodeId{ move(ua) };
	}
	Ω bytesNode( UA_UInt16 ns, const vector<uint8_t>& bytes )ι->NodeId{
		UA_NodeId ua{};
		ua.namespaceIndex = ns;
		ua.identifierType = UA_NODEIDTYPE_BYTESTRING;
		UA_ByteString_allocBuffer( &ua.identifier.byteString, bytes.size() );
		::memcpy( ua.identifier.byteString.data, bytes.data(), bytes.size() );
		return NodeId{ move(ua) };
	}
	constexpr UA_Guid nodeGuid{ 0x12345678, 0x1234, 0x5678, {0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78} };

	TEST( NodeIdTests, NumericIdentifier ){
		let n = numeric( 2, 5002 );
		EXPECT_TRUE( n.IsNumeric() );
		EXPECT_FALSE( n.IsString() );
		EXPECT_FALSE( n.IsGuid() );
		EXPECT_FALSE( n.IsBytes() );
		EXPECT_EQ( n.namespaceIndex, 2 );
		ASSERT_TRUE( n.Numeric() );
		EXPECT_EQ( *n.Numeric(), 5002u );
		EXPECT_FALSE( n.String() );
		EXPECT_FALSE( n.Guid() );
		EXPECT_FALSE( n.Bytes() );
		EXPECT_EQ( numeric(0, 7).namespaceIndex, 0 );
	}

	TEST( NodeIdTests, SystemNodes ){
		EXPECT_TRUE( NodeId::ObjectsFolder().IsSystem() );
		EXPECT_TRUE( numeric(0, 32750).IsSystem() );
		EXPECT_FALSE( numeric(0, 32751).IsSystem() );
		EXPECT_FALSE( numeric(2, 85).IsSystem() );  //a vendor namespace is never a system node...
		EXPECT_FALSE( fromJson(R"({"s":"tag"})").IsSystem() ); //...and neither is a non-numeric identifier.
	}

	//Every wrapper owns its identifier:  a copy allocates its own, a move leaves the source as the null node id.
	TEST( NodeIdTests, StringIdentifierOwnership ){
		let src = fromJson( R"({"ns":2,"s":"tag.one"})" );
		ASSERT_TRUE( src.IsString() );
		EXPECT_EQ( *src.String(), "tag.one" );

		NodeId copy{ src };
		EXPECT_EQ( *copy.String(), "tag.one" );
		EXPECT_NE( copy.identifier.string.data, src.identifier.string.data );

		NodeId source{ src };
		NodeId moved{ move(source) };
		EXPECT_EQ( *moved.String(), "tag.one" );
		EXPECT_TRUE( source.IsNumeric() ); //UA_NodeId_init leaves ns=0, i=0.
		EXPECT_EQ( *source.Numeric(), 0u );
		EXPECT_EQ( source.namespaceIndex, 0 );
	}

	TEST( NodeIdTests, Assignment ){
		NodeId a = fromJson( R"({"s":"a"})" );
		let b = fromJson( R"({"s":"b"})" );
		a = b;
		EXPECT_EQ( *a.String(), "b" );
		EXPECT_NE( a.identifier.string.data, b.identifier.string.data ); //a deep copy, not a shared buffer.

		NodeId c = fromJson( R"({"s":"c"})" );
		a = move( c );
		EXPECT_EQ( *a.String(), "c" );
		EXPECT_TRUE( c.IsNumeric() );

		auto& alias = a; //through a reference: `a = a` is -Wself-assign-overloaded, and the guard is what is under test.
		a = alias;
		EXPECT_EQ( *a.String(), "c" );
	}

	TEST( NodeIdTests, Ordering ){
		EXPECT_TRUE( numeric(0,1) < numeric(0,2) );
		EXPECT_FALSE( numeric(0,2) < numeric(0,1) );
		EXPECT_TRUE( numeric(0,5) < numeric(1,1) );  //namespace outranks the identifier.
		EXPECT_TRUE( numeric(0,5) == numeric(0,5) );
		EXPECT_FALSE( numeric(0,5) == numeric(1,5) );

		EXPECT_TRUE( fromJson(R"({"s":"a"})") < fromJson(R"({"s":"b"})") );
		EXPECT_TRUE( fromJson(R"({"s":"a"})") == fromJson(R"({"s":"a"})") );
		EXPECT_FALSE( numeric(0,1) == fromJson(R"({"s":"a"})") ); //identifierType breaks the tie.
		EXPECT_TRUE( numeric(0,1) < fromJson(R"({"s":"a"})") );   //NUMERIC(0) before STRING(3).
	}

	TEST( NodeIdTests, FromJsonForms ){
		EXPECT_EQ( *fromJson(R"({"ns":2,"i":5002})").Numeric(), 5002u );
		EXPECT_EQ( fromJson(R"({"ns":2,"i":5002})").namespaceIndex, 2 );
		EXPECT_EQ( *fromJson(R"({"s":"tag"})").String(), "tag" );
		EXPECT_EQ( fromJson(R"({"s":"tag"})").namespaceIndex, 0 );
		EXPECT_EQ( *fromJson("42").Numeric(), 42u );                        //a bare number is a numeric id in ns 0.
		EXPECT_EQ( *fromJson(R"("tag")").String(), "tag" );                 //a bare string is a string id in ns 0.
		EXPECT_EQ( *fromJson(R"({"ns":3,"id":{"i":7}})").Numeric(), 7u );   //a nested "id" inherits the outer namespace.
		EXPECT_EQ( fromJson(R"({"ns":3,"id":{"i":7}})").namespaceIndex, 3 );
		EXPECT_THROW( fromJson("true"), Exception );
	}

	TEST( NodeIdTests, GuidIdentifier ){
		let id = guidNode( 2, nodeGuid );
		ASSERT_TRUE( id.IsGuid() );
		ASSERT_TRUE( id.Guid() );
		EXPECT_EQ( Jde::ToString(*id.Guid()), "12345678-1234-5678-1234-567812345678" );
		EXPECT_EQ( id.ToJson().at("g").as_string(), "12345678-1234-5678-1234-567812345678" );
	}

	TEST( NodeIdTests, JsonRoundTrip ){
		for( let json : { R"({"ns":2,"i":5002})", R"({"ns":0,"i":85})", R"({"ns":2,"s":"tag.one"})", R"({"ns":0,"s":"x"})" } ){
			let original = fromJson( json );
			let round = NodeId{ jvalue{original.ToJson()} };
			EXPECT_TRUE( round==original ) << json;
		}
	}

	//DecodeJson takes the QL arg spelling - `ns:4,i:5002`, with or without braces - through QL::Parser.
	TEST( NodeIdTests, DecodeJsonQlForm ){
		let n = NodeId::DecodeJson( "ns:2,i:5002" );
		EXPECT_EQ( n.namespaceIndex, 2 );
		ASSERT_TRUE( n.Numeric() );
		EXPECT_EQ( *n.Numeric(), 5002u );

		let braced = NodeId::DecodeJson( R"({ns:2,s:"tag.one"})" );
		ASSERT_TRUE( braced.String() );
		EXPECT_EQ( *braced.String(), "tag.one" );
		EXPECT_EQ( braced.namespaceIndex, 2 );
	}

	//reviews/opc-review2.md #9 reports that ToString returns the fragment `"Id":5002,"Namespace":2` because
	//options.stringNodeIds is left unset.  That does not reproduce against the installed open62541: a value-initialized
	//UA_EncodeJsonOptions already encodes a NodeId as the quoted string form, so the substr(1,size-2) strips the quotes
	//it means to.  Enabled, as the regression guard for whatever the encoder default does next.
	TEST( NodeIdTests, ToStringEmitsTheUaStringForm ){
		EXPECT_EQ( numeric(2, 5002).ToString(), "ns=2;i=5002" );
		EXPECT_EQ( numeric(0, 85).ToString(), "i=85" );
		EXPECT_EQ( fromJson(R"({"ns":2,"s":"tag.one"})").ToString(), "ns=2;s=tag.one" );
	}

	TEST( NodeIdTests, ToStringOfAList ){
		vector<NodeId> ids;
		ids.emplace_back( numeric(2, 5002) );
		ids.emplace_back( fromJson(R"({"ns":2,"s":"tag"})") );
		EXPECT_EQ( NodeId::ToString(ids), R"([{"ns":2,"i":5002},{"ns":2,"s":"tag"}])" );
	}

	TEST( NodeIdTests, InsertParamsNumeric ){
		let params = numeric( 2, 5002 ).InsertParams();
		ASSERT_EQ( params.size(), 5u );
		EXPECT_EQ( params[0].ToUInt(), 2u );    //namespaceIndex
		EXPECT_EQ( params[1].ToUInt(), 5002u ); //numeric
		EXPECT_TRUE( params[2].is_null() );     //string
	}

	TEST( NodeIdTests, InsertParamsString ){
		let params = fromJson( R"({"ns":2,"s":"tag.one"})" ).InsertParams();
		ASSERT_EQ( params.size(), 5u );
		EXPECT_EQ( params[0].ToUInt(), 2u );
		EXPECT_TRUE( params[1].is_null() );
		EXPECT_EQ( params[2].get_string(), "tag.one" );
	}

	TEST( NodeIdTests, InsertParamsGuid ){
		let params = guidNode( 2, nodeGuid ).InsertParams();
		ASSERT_EQ( params.size(), 5u );
		EXPECT_TRUE( params[1].is_null() );
		EXPECT_TRUE( params[2].is_null() );
		ASSERT_EQ( params[3].get_bytes().size(), 16u );
		EXPECT_EQ( params[3].get_guid(), ToGuid(nodeGuid) );
	}

	TEST( NodeIdTests, InsertParamsBytes ){
		const vector<uint8_t> bytes{ 0xde, 0xad, 0xbe, 0xef };
		let params = bytesNode( 2, bytes ).InsertParams();
		ASSERT_EQ( params.size(), 5u );
		EXPECT_TRUE( params[1].is_null() );
		EXPECT_TRUE( params[2].is_null() );
		EXPECT_EQ( params[4].get_bytes(), bytes );
	}

	// ---- the review's open findings.  See main.cpp for why these are disabled. -------------------------------------

	//#2: NodeId converts implicitly to its owning UA_NodeId base but offers no way to hand ownership over, so every
	//`raw.nodeId = move(wrapper)` in the consumers compiles as a shallow copy through the sliced base and the wrapper
	//still frees the identifier the in-flight request points at.  Variant::Move()/ExNodeId::Move() are the shape to copy.
	template<class T> concept HasMove = requires( T t ){ { t.Move() } -> std::same_as<UA_NodeId>; };
	TEST( NodeIdTests, DISABLED_R2_2_ExposesMove ){
		EXPECT_TRUE( HasMove<NodeId> );
	}

	//#4: DecodeJson hands UA_decodeJson a `NodeId*` where a `UA_NodeId*` is expected.  NodeId is polymorphic (virtual
	//dtor), so the base sits at offset 8 and the implicit conversion to void* performs no adjustment - the decoder writes
	//its 24-byte image over the vptr and the returned node carries a bogus identifier.  ToString is fine (see the enabled
	//test above), so this is purely the missing static_cast; the two are each other's inverse and must round trip.
	//Enabling it before the fix corrupts the heap.
	TEST( NodeIdTests, DISABLED_R2_4_DecodeJsonRoundTripsToString ){
		for( let& n : { numeric(2, 5002), numeric(0, 85), fromJson(R"({"ns":2,"s":"tag.one"})") } )
			EXPECT_TRUE( NodeId::DecodeJson(n.ToString())==n ) << n.ToString();
	}

	//#7: the serializer emits lowercase hex for a byte-string identifier while FromJson base64-decodes it, so a node
	//cannot re-read its own json - BadNodeIdUnknown, or a hit on the wrong node when the hex parses as valid base64.
	//Whichever encoding wins, this has to round trip.
	TEST( NodeIdTests, DISABLED_R2_7_ByteStringJsonRoundTrip ){
		const vector<uint8_t> bytes{ 0xde, 0xad, 0xbe, 0xef };
		let original = bytesNode( 2, bytes );
		ASSERT_TRUE( original.IsBytes() );
		let round = NodeId{ jvalue{original.ToJson()} };
		EXPECT_TRUE( round==original ) << serialize( original.ToJson() );
	}

	//#6: the unused guid/bytes columns are bound as a *non-null* empty blob (DB::Value{vector<uint8_t>{}}) while the
	//insert procs dispatch on IS NULL, so a byte-string id takes the guid branch and dies in toUuidString with
	//"Guid blob is 0 bytes, expected 16."  The numeric/string params two lines up already use DB::Value{}.
	TEST( NodeIdTests, DISABLED_R2_6_UnusedIdentifierColumnsAreNull ){
		let numericParams = numeric( 2, 5002 ).InsertParams();
		EXPECT_TRUE( numericParams[3].is_null() ) << "guid column of a numeric id";
		EXPECT_TRUE( numericParams[4].is_null() ) << "bytes column of a numeric id";

		let bytesParams = bytesNode( 2, {0xde, 0xad} ).InsertParams();
		EXPECT_TRUE( bytesParams[3].is_null() ) << "guid column of a byte-string id";
		EXPECT_FALSE( bytesParams[4].is_null() );

		let guidParams = guidNode( 2, nodeGuid ).InsertParams();
		EXPECT_TRUE( guidParams[4].is_null() ) << "bytes column of a guid id";
		EXPECT_FALSE( guidParams[3].is_null() );
	}

	//#11: open62541 treats a non-empty outBuf as a hard limit ("if sufficient"), so ToString's 1024-byte buffer caps the
	//identifier length.  Confirmed: a 40-char id gives `s=yyy...`, a 2048-char one silently falls back to the *other*
	//textual format, serialize(ToJson()) - `{"ns":0,"s":"xxx..."}` - which a DecodeJson round trip then reads back as a
	//different node.  The assertion has to check the format, not just that the identifier appears somewhere: the
	//fallback contains it too.  Pass an empty UA_String and let the encoder size it.
	TEST( NodeIdTests, DISABLED_R2_11_LongStringIdentifierSurvivesToString ){
		let identifier = string( 2048, 'x' );
		let id = NodeId{ jvalue{jstring{identifier}} };
		ASSERT_TRUE( id.IsString() );
		EXPECT_EQ( id.ToString(), "s="+identifier ) << id.ToString().substr( 0, 64 );
	}
}
