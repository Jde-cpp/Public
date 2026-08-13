#include "jde/fwk/log/ILogger.h"
#include <jde/fwk/log/MemoryLog.h>
#define let const auto
#pragma warning( disable: 4702 )

namespace Jde::Tests{
	struct LogGeneralTests : public ::testing::Test{
	protected:
		LogGeneralTests() {}
		~LogGeneralTests() override{}

		Ω SetUpTestCase()ι->void{ }
		α SetUp()->void override{ Logging::ClearMemory(); }
		α TearDown()->void override {}
	};

	//ELogTags is a 64-bit space that libraries extend above fwk's own 27 bits - Jde::Opc registers 32-45 through
	//AddTagParser - so All has to span the whole underlying type.  Spelled ~0ul it stopped at bit 31 under the windows
	//ABI, where unsigned long is 32-bit, and silently dropped every extension tag from an All-based subscription.
	TEST_F( LogGeneralTests, AllCoversTheFullUnderlyingWidth ){
		EXPECT_EQ( (uint)ELogTags::All, ~(std::underlying_type_t<ELogTags>)0 );
		EXPECT_NE( (uint)ELogTags::All & (1ull<<32), 0ull );//the bit range that actually went missing.
	}

	//AddTagParser takes ownership as up<ITagParser>, so every parser is destroyed through a base pointer.  Without a
	//virtual destructor on the base that is undefined behaviour, and in practice the derived destructor never runs - it
	//goes unnoticed only because the one implementation, Jde::Opc::UALogParser, holds no state to leak.
	TEST_F( LogGeneralTests, TagParserIsDestroyedThroughTheBase ){
		static bool destroyed;//a local class may use a static local of its enclosing function, not an automatic one.
		struct Counted : Logging::ITagParser{
			~Counted(){ destroyed = true; }
			α ToTag( str )Ι->ELogTags override{ return ELogTags::None; }
			α ToString( ELogTags )Ι->string override{ return {}; }
			α Tags()Ι->flat_map<string,uint> override{ return {}; }
		};
		destroyed = false;
		up<Logging::ITagParser> parser = mu<Counted>();//the conversion AddTagParser performs.
		parser.reset();
		EXPECT_TRUE( destroyed );
	}

	//config keys are split on TagSeparator and each part looked up in ELogTagStrings, so an unrecognised part is simply
	//dropped and the key silently collapses onto a shorter one's mask - where parseTags' insert_or_assign overwrites it.
	TEST_F( LogGeneralTests, CompositeTagNamesResolveDistinctly ){
		let tags = []( sv name ){ return ToLogTags( name ); };//sv, not a literal - ToLogTags(sv)/ToLogTags(jvalue) are ambiguous for const char*.
		EXPECT_EQ( tags("socket.client.read.subscription"), ELogTags::SocketClientReadSub );
		EXPECT_EQ( tags("socket.client.write.subscription"), ELogTags::SocketClientWriteSub );
		//the collision the ".sub" spelling caused: it must not resolve to the plain read/write tag.
		EXPECT_NE( tags("socket.client.read.subscription"), tags("socket.client.read") );
		EXPECT_NE( tags("socket.client.write.subscription"), tags("socket.client.write") );
		EXPECT_EQ( tags("socket.client.read"), ELogTags::SocketClientRead );
		//an unknown part contributes nothing, which is exactly how ".sub" aliased onto socket.client.read.
		EXPECT_EQ( tags("socket.client.read.sub"), ELogTags::SocketClientRead );
		//and the old separator is now just an unknown name, not a second spelling of the same tag.
		EXPECT_EQ( tags("socket_client_read"), ELogTags::None );
	}

	//tags travel as a json *value*: one tag is the bare string, several are an array, and ToLogTags reads either back.
	//`jvalue{tagString}` does not spell the first one - value's initializer_list constructor wins over the string one and
	//wraps it in an array - so a single tag came back as ELogTagStrings[0], "none", whatever tag it actually was.
	TEST_F( LogGeneralTests, ToValueSpellsOneTagBare ){
		EXPECT_EQ( ToValue(ELogTags::Sql).as_string(), "sql" );
		EXPECT_EQ( ToString(ELogTags::Sql, false), "sql" );
		EXPECT_EQ( ToValue(ELogTags::None).as_string(), "none" );
		let multi = ToValue( ELogTags::SocketClientRead );
		ASSERT_TRUE( multi.is_array() );
		EXPECT_EQ( multi.as_array().size(), 3u );//no tag parsers here, so socket|client|read is exactly its three fwk names.
		EXPECT_EQ( ToLogTags(multi), ELogTags::SocketClientRead );
		EXPECT_EQ( ToLogTags(ToValue(ELogTags::Sql)), ELogTags::Sql );
	}

	//instanceTagLevel groups the tags under their level, since a combined tag is only spellable as an array and an array
	//is no object key; SetLevels and the config file key on the tag.  The flip between them has to be lossless, or an
	//instance silently drops at startup the overrides the app server just handed it.
	TEST_F( LogGeneralTests, ToTagLevelsFlipsTheGrouping ){
		jobject grouped;
		grouped["Debug"] = jarray{ ToValue(ELogTags::Sql), ToValue(ELogTags::SocketClientRead) };
		grouped["Warning"] = jarray{ jstring{"default"} };
		let flat = ToTagLevels( grouped );
		ASSERT_EQ( flat.size(), 3u );
		EXPECT_EQ( flat.at("sql").as_string(), "Debug" );
		EXPECT_EQ( flat.at("default").as_string(), "Warning" ) << "the default level rides as the 'default' tag - parseTags looks for that exact spelling";
		auto found = false;//the combined tag joins back with the separator ToLogTags splits on, whatever order the bits came out in.
		for( let& kv : flat )
			found |= kv.key()!="sql" && kv.key()!="default" && ToLogTags(sv{kv.key()})==ELogTags::SocketClientRead;
		EXPECT_TRUE( found );
	}

	//updateInstanceTagLevel takes a record per override for the same reason: `tags` is a value, so a combined tag can be
	//an array there too.  Either spelling of it arrives - the parts, or the joined name - and both flatten the same.
	TEST_F( LogGeneralTests, ToTagLevelsReadsTheMutationArgument ){
		let flat = ToTagLevels( jarray{
			jobject{ {"tags", jarray{jstring{"socket"},jstring{"client"},jstring{"read"}}}, {"level", "Debug"} },
			jobject{ {"tags", jarray{jstring{"socket.client.write"}}}, {"level", "Error"} },
			jobject{ {"tags", jarray{jstring{"crypto"}}}, {"level", nullptr} },
			jobject{ {"tags", jarray{jstring{"parsing"}}} }//no level at all - the same "remove this override" a null is.
		} );
		ASSERT_EQ( flat.size(), 4u );
		EXPECT_TRUE( flat.contains("socket.client.read") ) << "the parts join with the separator ToLogTags splits on";
		EXPECT_EQ( flat.at("socket.client.write").as_string(), "Error" );
		EXPECT_TRUE( flat.at("crypto").is_null() );
		EXPECT_TRUE( flat.at("parsing").is_null() );

		let roundTrip = ToTagLevels( ToTagLevelArray(flat) );//and back out to the wire and in again.
		EXPECT_EQ( serialize(roundTrip), serialize(flat) );
	}

	TEST_F( LogGeneralTests, CachedTags ){
		auto& logger = Logging::GetLogger<Logging::MemoryLog>();

		let _tags = ELogTags::Scheduler;
		Logging::ClearMemory();
		constexpr auto logMessage = "scheduler msg";
		TRACE( logMessage );
		ASSERT_TRUE( logger.Find(logMessage).empty() );
		logger.SetLevel( _tags, ELogLevel::Debug );
		DBG( logMessage );
		ASSERT_FALSE( logger.Find(logMessage).empty() );
	}

	TEST_F( LogGeneralTests, ArgsNotCalled ){
		auto& logger = Logging::GetLogger<Logging::MemoryLog>();
		let unConfiguredTags = ELogTags::Scheduler;
		logger.SetLevel( unConfiguredTags, ELogLevel::Error );
		auto arg = []()->string {
			throw std::runtime_error("should not be called");
			return "";
		};
		ASSERT_NO_THROW( TRACET(unConfiguredTags, "{}", arg()) );
	}

	//TODO makesure cumulative is updated when some obscure tag is sent.
	//How quick is md5 calculation.
	//How quick is noop.
	//How quick is memory.
	//How quick is spdlog.
}
