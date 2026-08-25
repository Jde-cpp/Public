#include "../src/uaTypes/Node.h"
#define let const auto

namespace Jde::Opc::Server::Tests{
	//Node is abstract and every concrete subclass drags in its UA_*Attributes; the id is what is under test, so the
	//subclass here is the minimum that makes `Node( jobject, … )` constructible.
	struct TestNode final : Node{
		using Node::Node;
		α Specified()Ι->UA_UInt32 override{ return 0; }
		α Name()Ι->UA_LocalizedText override{ return UA_LocalizedText{}; }
		α Description()Ι->UA_LocalizedText override{ return UA_LocalizedText{}; }
		α WriteMask()Ι->UA_UInt32 override{ return 0; }
		α UserWriteMask()Ι->UA_UInt32 override{ return 0; }
	};
	struct NodeTests : ::testing::Test{};

	//opc-review3 #1:  the ternary used to be `j.contains("id") ? NodeId{j.at("id")} : UA_NodeId{}`, whose common type is
	//the *base*, so the wrapper temporary was sliced - `NodeId( UA_NodeId&& )` took the identifier and the temporary's
	//~NodeId freed it.  A numeric id has no heap identifier and masks the whole thing; a string id is the pin: reverted
	//onto the old line, ASan reports a heap-use-after-free on the read below, freed by ~NodeId inside Node::Node.
	TEST_F( NodeTests, StringIdSurvivesTheCtorTernary ){
		let node = TestNode{ jobject{{"id", "tag"}}, 0, BrowseName{} };
		ASSERT_TRUE( node.IsString() );
		EXPECT_EQ( "tag", node.String().value_or(string{}) );
		EXPECT_EQ( 0u, node.PK );//a string id is not a pk.
	}

	TEST_F( NodeTests, NoIdIsANullNodeId ){
		let node = TestNode{ jobject{}, 0, BrowseName{} };
		EXPECT_TRUE( node.IsNumeric() );
		EXPECT_EQ( 0u, node.identifier.numeric );
	}
}
