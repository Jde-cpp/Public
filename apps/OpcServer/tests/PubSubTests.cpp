#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <NodesetLoader/backendOpen62541.h>
#include <jde/fwk/settings.h>
#include <jde/opc/UAException.h>
#include <jde/opc/pubsub/PubSub.h>
#include "../src/globals.h"
#include "../src/UAServer.h"
#include "../src/pubsub/PubSubReader.h"

#define let const auto
namespace Jde::Opc::Server::Tests{
	//The pumps nodeset the tests config lists under /opcServer/configFiles - the same file the emulator loads.
	Ω pumpsNodeset()ε->fs::path{
		for( let& p : Settings::FindPathArray("/opcServer/configFiles") ){
			if( p.filename()=="pumps.NodeSet2.xml" )
				return p;
		}
		THROW( "pumps.NodeSet2.xml is not in /opcServer/configFiles." );
	}
	//An in-process Part 14 publisher: its own headless UA_Server holding the pumps nodeset, publishing the contract the
	//OpcServer reads - what the PLC emulator does, minus the emulator.  Port 4850 keeps it off the OpcServer's 4840.
	struct Publisher final : noncopyable{
		Publisher( const jobject& settings, const fs::path& nodeset )ε:
			Contract{ settings }{
			UA_ServerConfig config{};
			UAε( UA_ServerConfig_setMinimal(&config, 4850, nullptr) );
			_server = UA_Server_newWithConfig( &config );
			THROW_IF( !_server, "UA_Server_newWithConfig failed." );
			THROW_IF( !NodesetLoader_loadFile(_server, nodeset.string().c_str(), nullptr), "Publisher could not load '{}'.", nodeset.string() );
			Contract.Resolve( *_server );
			_writer = mu<PubSub::Writer>( *_server, Contract );
			UAε( UA_Server_run_startup(_server) );
		}
		~Publisher(){
			UA_Server_run_shutdown( _server );
			UA_Server_delete( _server );
		}
		α Iterate()ι->void{ UA_Server_run_iterate( _server, false ); }
		α Write( uint field, double value )ε->void{
			UA_Variant v;
			UA_Variant_setScalar( &v, &value, &UA_TYPES[UA_TYPES_DOUBLE] );
			UAε( UA_Server_writeValue(_server, Contract.Fields[field].Node, v) );
		}
		PubSub::Config Contract;
	private:
		UA_Server* _server{};
		up<PubSub::Writer> _writer;
	};

	struct PubSubTests : ::testing::Test{
	protected:
		//Startup already built a reader on its server, but sibling fixtures replace the global UAServer (Initialize), so
		//rebuild the OpcServer side here exactly as opcServerStartup does: load, run, subscribe.
		Ω SetUpTestCase()ε->void{
			Server::Initialize( GetSchemaPtr() );
			auto& ua = GetUAServer();
			ua.Load( pumpsNodeset() );
			ua.Run();
			StartPubSub( Settings::AsObject("/opcServer/pubsub") );
		}
		Ω ReadDouble( const NodeId& node )ι->optional<double>{
			optional<double> y;
			UA_Variant v; UA_Variant_init( &v );
			if( UA_Server_readValue(GetUAServer().Ptr(), node, &v)==UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_DOUBLE]) )
				y = *(UA_Double*)v.data;
			UA_Variant_clear( &v );
			return y;
		}
	};

	TEST_F( PubSubTests, ReaderTargetsResolveToTheNodeset ){
		let reader = PubSub(); ASSERT_TRUE( reader );
		let& contract = reader->Config();
		ASSERT_EQ( contract.Fields.size(), 5u );
		EXPECT_EQ( contract.Fields[0].Name, "pump1.motorRpm" );
		EXPECT_EQ( *contract.Fields[0].Node.Numeric(), 6012u );
		EXPECT_EQ( *contract.Fields[4].Node.Numeric(), 6054u );
		for( let& f : contract.Fields )
			EXPECT_EQ( f.Type, &UA_TYPES[UA_TYPES_DOUBLE] ) << f.Name;
	}

	//A published value lands in the OpcServer's target variable: the whole UADP path - writer sampling, network message,
	//reader decode against the shared metadata, TargetVariables write.
	TEST_F( PubSubTests, PublishedValueLandsInTargetVariable ){
		let& contract = PubSub()->Config();
		Publisher publisher{ Settings::AsObject("/opcServer/pubsub"), pumpsNodeset() };
		constexpr double expected{ 1234.5 };
		publisher.Write( 0, expected );
		publisher.Write( 4, expected*2 );
		optional<double> got, gotManual;
		for( let deadline = steady_clock::now()+10s; steady_clock::now()<deadline; std::this_thread::sleep_for(50ms) ){
			publisher.Iterate();
			got = ReadDouble( contract.Fields[0].Node );
			gotManual = ReadDouble( contract.Fields[4].Node );
			if( got && *got==expected && gotManual && *gotManual==expected*2 )
				break;
		}
		ASSERT_TRUE( got ) << "pump1.motorRpm never became readable";
		EXPECT_EQ( *got, expected );
		ASSERT_TRUE( gotManual );
		EXPECT_EQ( *gotManual, expected*2 );
	}
}
