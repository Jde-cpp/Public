#include <jde/web/Jwt.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include "../src/GatewayAppClient.h"
#include "../src/UAClient.h"
#include "../src/StartupAwait.h"
#include "../src/ql/NodeQLAwait.h"
#include "helpers.h"
#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };
	struct BrowseTests : ::testing::Test{
	protected:
		Ω SetUpTestCase()ι->void{
			_jwt = BlockAwait<Web::Client::ClientSocketAwait<Jde::Web::Jwt>,Web::Jwt>( AppClient()->Jwt() );
			auto sessionId = *Str::TryTo<SessionPK>(_jwt->SessionId, nullptr, 16);
			TRACE( "UserPK: {:x}, SessionId: {:x}", _jwt->UserPK.Value, sessionId );
			auto con = GetConnection( OpcServerTarget );
			Credential cred{ _jwt->Payload() };
			_client = BlockAwait<TAwait<sp<UAClient>>,sp<UAClient>>( ConnectAwait{move(con.Target), cred} );
			AddSession( sessionId, OpcServerTarget, move(cred) );
		};
		Ω TearDownTestCase()ι->void{
			UAClient::RemoveClient( move(_client) );
		}
		α SetUp()ι->void{}

		static optional<Web::Jwt> _jwt;
		static sp<UAClient> _client;
	};
	optional<Web::Jwt> BrowseTests::_jwt;
	sp<UAClient> BrowseTests::_client;

	TEST_F( BrowseTests, NodeId ){
		auto query = Ƒ("node(opc: $opc, path:$path){{id name description dataType}}");
		jobject variables{ {"opc", OpcServerTarget}, {"path", "4~Examples/4~Stacklights/4~ExampleStacklight"} };
		auto ql = QL::Parse( query, move(variables), Schemas(), true );
		auto value = BlockAwait<NodeQLAwait, jvalue>( NodeQLAwait{move(ql.Queries().front()), *Str::TryTo<SessionPK>(_jwt->SessionId, nullptr, 16), _jwt->UserPK} );
		TRACE( "value: {}", serialize(value) );
		auto result = ExNodeId{ value };
		ASSERT_TRUE( *result.Numeric()>0 );
		ASSERT_EQ( value.at("name"), "Pump (Manual)" );
		ASSERT_EQ( value.at("description"), "Test Explicit Object" );

/*		let path = "pumpManual"s;
		//let path2 = "Server|Objects|1:Devices|1:DeviceSet|1:Device1|1:Pumps|1:Pump1|1:Parameters|1:pumpManual";

    #define BROWSE_PATHS_SIZE 1
    string paths[BROWSE_PATHS_SIZE] = { path };
    UA_UInt32 ids[BROWSE_PATHS_SIZE] = {UA_NS0ID_ORGANIZES};
    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(BROWSE_PATHS_SIZE, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
    browsePath.relativePath.elementsSize = BROWSE_PATHS_SIZE;

    for(size_t i = 0; i < BROWSE_PATHS_SIZE; i++) {
			UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
			elem->referenceTypeId = UA_NODEID_NUMERIC(0, ids[i]);
			elem->targetName = UA_QUALIFIEDNAME_ALLOC( 1, paths[i].c_str() );
    }

    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePaths = &browsePath;
    request.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse response = UA_Client_Service_translateBrowsePathsToNodeIds(*_client, request);
    //ASSERT_GOOD(response.responseHeader.serviceResult);
		std::cout << "found " << response.resultsSize << " nodes with status " << UA_StatusCode_name(response.results->statusCode) << std::endl;
    UA_BrowsePath_clear(&browsePath);
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&response);

/ *		UA_RelativePathElement rpe{};
		UA_RelativePathElement_init(&rpe);
		rpe.referenceTypeId = UA_NODEID_NUMERIC(0,UA_NS0ID_ORGANIZES);
		rpe.isInverse = false;
		rpe.includeSubtypes = false;
		rpe.targetName = UA_QualifiedName{ 0, ToUV(path) };

		UA_RelativePathElement elements[] = {rpe};

		// set the relative path data
		UA_RelativePath relative_path{};
		UA_RelativePath_init(&relative_path);
		relative_path.elementsSize = 1;
		relative_path.elements = elements;

		// create a browse path with a startingNode which in this example case is an object on the server
		UA_BrowsePath bp{};
		UA_BrowsePath_init(&bp);
		bp.startingNode = UA_NODEID_NUMERIC(0,UA_NS0ID_OBJECTSFOLDER);
		bp.relativePath = relative_path;

		UA_BrowsePath paths[] = {bp};

		UA_TranslateBrowsePathsToNodeIdsRequest request{};
		UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
		request.browsePathsSize = 1;
		request.browsePaths = paths;

		// request the translation
		UA_TranslateBrowsePathsToNodeIdsResponse response = UA_Client_Service_translateBrowsePathsToNodeIds( *_client, request );* /
		//std::cout << "found " << response.resultsSize << " nodes with status " << UA_StatusCode_name(response.results->statusCode) << std::endl;
*/
	}
}