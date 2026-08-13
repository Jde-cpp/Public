//Ported from apps/OpcGateway/tests/LogSettingTests.cpp - the same "read the log settings, flip a default, read it back"
//round trip, driven against the awaitables in Jde.App.Shared instead of a running gateway.  LogSettingsAwait resolves in
//await_ready and LogSettingsMAwait's Resume/ResumeExp are synchronous, so BlockAwait runs both on this thread.
#include <gtest/gtest.h>
#include <jde/fwk/log/SpdLog.h>
#include <jde/ql/types/RequestQL.h>
#include <jde/app/IApp.h>
#include <jde/app/log/LogSettingsAwait.h>
#include <jde/app/log/ProtoLog.h>
#include "helpers.h"

#define let const auto

namespace Jde::App::Tests{
	//Resolves in await_ready, so the mutation's forward to the app server never leaves this thread.
	struct ValueAwait final : TAwait<jvalue>{
		ValueAwait( jvalue value, SRCE )ι:TAwait<jvalue>{sl}, _value{move(value)}{}
		α await_ready()ι->bool override{ return true; }
		α Suspend()ι->void override{ ASSERT(false); }
		α await_resume()ε->jvalue override{ return move(_value); }
	private:
		jvalue _value;
	};

	//Only the query hand-off is reachable from here;  every other IApp member is a hard error if the mutation ever calls it.
	struct AppStub final : IApp{
		α PublicKey()Ι->const Crypto::PublicKey& override{ static const Crypto::PublicKey y; return y; }
		α SessionInfoAwait( SessionPK, SL )ε->up<TAwait<Web::FromServer::SessionInfo>> override{ throw Exception{"AppStub::SessionInfoAwait"}; }
		α Login( Web::Jwt&&, SL )ε->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo> override{ throw Exception{"AppStub::Login"}; }
		α ClientQuery( QL::RequestQL&&, UserPK, SL )ε->up<TAwait<jvalue>> override{ throw Exception{"AppStub::ClientQuery"}; }

		string Query;      //what the mutation forwarded to the app server.
		jobject Variables;
		uint Queries{};
	private:
		α QueryArray( string&&, jobject, bool, SL )ε->up<TAwait<jarray>> override{ throw Exception{"AppStub::QueryArray"}; }
		α QueryObject( string&&, jobject, bool, SL )ε->up<TAwait<jobject>> override{ throw Exception{"AppStub::QueryObject"}; }
		α QueryValue( string&& q, jobject variables, bool, SL )ε->up<TAwait<jvalue>> override{
			Query = move( q );
			Variables = move( variables );
			++Queries;
			return mu<ValueAwait>( jvalue{true} );
		}
	};

	//logSetting{ text binary tags }
	Ω logSettings( std::initializer_list<sv> columns={"text","binary","tags"} )ε->jobject{
		auto ql = table( "logSetting" );
		addColumns( ql, columns );
		let y = BlockAwait<LogSettingsAwait,jvalue>( LogSettingsAwait{move(ql)} );
		return y.as_object();
	}
	Ω defaultLevel( sv logger )ε->ELogLevel{ return ToLogLevel( logSettings().at(logger).at("default").as_string() ); }

	//updateLogSetting( text:{default: $default} )
	Ω updateLogSettings( sv args, jobject variables, sp<AppStub> app )ε->jvalue{
		static const vector<sp<DB::AppSchema>> noSchemas;
		QL::MutationQL m{ "updateLogSetting", QL::Parser::ParseArgs(string{args}), ms<jobject>(move(variables)), optional<QL::TableQL>{}, true, noSchemas, true };
		return BlockAwait<LogSettingsMAwait,jvalue>( LogSettingsMAwait{move(m), app, UserPK{}} );
	}

	struct LogSettingsTests : ::testing::Test{
	protected:
		α SetUp()->void override{
			_text = Logging::FindLogger<Logging::SpdLog>();
			_binary = Logging::FindLogger<App::ProtoLog>();
			ASSERT_TRUE( _text );
			ASSERT_TRUE( _binary ); //config/App.Tests.jsonnet configures /logging/proto - without it the binary half is vacuous.
			_textDefault = _text->DefaultLevel();
			_binaryDefault = _binary->DefaultLevel();
		}
		α TearDown()->void override{ //process-wide loggers - put back what the other suites' log output depends on.
			_text->SetDefaultLevel( _textDefault );
			_binary->SetDefaultLevel( _binaryDefault );
		}
		Logging::SpdLog* _text{};
		App::ProtoLog* _binary{};
		ELogLevel _textDefault{};
		ELogLevel _binaryDefault{};
	};

	TEST_F( LogSettingsTests, QueryReturnsEachRequestedLogger ){
		let y = logSettings();
		ASSERT_TRUE( y.contains("text") );
		ASSERT_TRUE( y.contains("binary") );
		ASSERT_TRUE( y.contains("tags") );
		EXPECT_EQ( ToLogLevel(y.at("text").as_object().at("default").as_string()), _text->DefaultLevel() );
		EXPECT_EQ( ToLogLevel(y.at("binary").as_object().at("default").as_string()), _binary->DefaultLevel() );
	}

	//Column-driven, like every other QL result:  what was not asked for is not in the object.
	TEST_F( LogSettingsTests, QueryOmitsUnrequestedColumns ){
		let y = logSettings( {"text"} );
		EXPECT_TRUE( y.contains("text") );
		EXPECT_FALSE( y.contains("binary") );
		EXPECT_FALSE( y.contains("tags") );
		EXPECT_TRUE( logSettings({}).empty() );
	}

	//Each logger reports the per-tag levels it was configured with, spelled the way the settings spell them.
	TEST_F( LogSettingsTests, QueryReportsConfiguredTagLevels ){
		let text = logSettings( {"text"} ).at( "text" ).as_object();
		EXPECT_EQ( text.at("app").as_string(), "Trace" );        //logging.spd.tags in App.Tests.jsonnet…
		EXPECT_EQ( text.at("settings").as_string(), "Debug" );
		let binary = logSettings( {"binary"} ).at( "binary" ).as_object();
		EXPECT_EQ( binary.at("externalLogger").as_string(), "None" ); //…and logging.proto.tags.
	}

	//`tags` is the name->bit map the settings UI builds its tag list from, not a per-logger level.  The query asks for
	//the *user* catalog - Tags(true) - which adds the composite tags (http.server.read = Http|Server|Read &c) on top of
	//the single-bit ones, so assert against that overload, not the bare Tags().
	TEST_F( LogSettingsTests, QueryReturnsTheTagCatalog ){
		let tags = logSettings( {"tags"} ).at( "tags" ).as_object();
		EXPECT_EQ( tags.size(), Logging::Tags(true).size() );
		EXPECT_GT( Logging::Tags(true).size(), Logging::Tags().size() ) << "the user catalog is a superset of the single-bit tags";
		EXPECT_EQ( tags.at("app").to_number<uint>(), (uint)ELogTags::App );
		EXPECT_EQ( tags.at("test").to_number<uint>(), (uint)ELogTags::Test );
		//a catalogue name the ui cannot save is worse than no name: it must be the spelling ToLogTags reads and the one
		//the logSetting/instanceTagLevel answers carry, which is ToString's - bit order, not the config's reading order.
		let composite = Jde::ToString( ELogTags::HttpServerRead, false );
		EXPECT_EQ( tags.at(composite).to_number<uint>(), (uint)ELogTags::HttpServerRead ) << "composite tags have to reach the UI";
		EXPECT_EQ( ToLogTags(sv{composite}), ELogTags::HttpServerRead ) << "and have to come back";
		for( let& [name, id] : tags )
			EXPECT_EQ( underlying(ToLogTags(sv{name})), id.to_number<uint>() ) << "every catalogue name round-trips: " << name;
	}

	//The ported round trip:  flip the default, read it back through the query, for each logger in turn.
	TEST_F( LogSettingsTests, UpdateDefault ){
		auto app = ms<AppStub>();
		app->SetAppPKs( 42, 1 );
		let testUpdate = [&]( sv logger, ELogLevel current ){
			let updated = current==ELogLevel::Information ? ELogLevel::Warning : ELogLevel::Information;
			updateLogSettings( Ƒ("{{{}: {{default: $default}}}}", logger), jobject{{"default", ToString(updated)}}, app );
			EXPECT_EQ( defaultLevel(logger), updated );
		};
		testUpdate( "text", _text->DefaultLevel() );
		testUpdate( "binary", _binary->DefaultLevel() );
	}

	TEST_F( LogSettingsTests, UpdateSetsATagLevel ){
		auto app = ms<AppStub>();
		app->SetAppPKs( 42, 1 );
		updateLogSettings( R"({text: {ql: "Critical"}})", {}, app );
		EXPECT_EQ( logSettings({"text"}).at("text").as_object().at("ql").as_string(), "Critical" );
		EXPECT_EQ( _text->DefaultLevel(), _textDefault ); //a tag level is not the default.
	}

	//The runtime update and the app-server hand-off are two halves of one mutation:  the levels change here, and the same
	//args are forwarded as the instance's stored tag levels so they survive a restart.
	TEST_F( LogSettingsTests, UpdateForwardsToTheAppServer ){
		auto app = ms<AppStub>();
		app->SetAppPKs( 42, 1 );
		updateLogSettings( R"({text: {default: "Warning"}})", {}, app );
		ASSERT_EQ( app->Queries, 1u );
		EXPECT_NE( app->Query.find("updateInstanceTagLevel"), string::npos ); //re-addressed from updateLogSetting…
		EXPECT_NE( app->Query.find("\"id\":42"), string::npos );              //…at this instance's pk.
		EXPECT_NE( app->Query.find("Warning"), string::npos );
	}

	//No instance pk means nothing to store against, so the mutation fails - but only after the in-process levels are set,
	//which is what a gateway that has not finished connecting still needs.
	TEST_F( LogSettingsTests, UpdateWithoutAnInstanceStillSetsTheRuntimeLevels ){
		auto app = ms<AppStub>(); //no SetAppPKs - InstancePK() is 0.
		EXPECT_THROW( updateLogSettings(R"({text: {default: "Critical"}})", {}, app), Exception );
		EXPECT_EQ( app->Queries, 0u );
		EXPECT_EQ( defaultLevel("text"), ELogLevel::Critical );
	}

	TEST_F( LogSettingsTests, IsApplicable ){
		static const vector<sp<DB::AppSchema>> noSchemas;
		let mutation = []( sv command )ε{ return QL::MutationQL{ string{command}, jobject{}, ms<jobject>(), optional<QL::TableQL>{}, true, noSchemas, true }; };
		EXPECT_TRUE( LogSettingsMAwait::IsApplicable(mutation("updateLogSetting")) );
		EXPECT_TRUE( LogSettingsMAwait::IsApplicable(mutation("updateLogSettings")) );
		EXPECT_FALSE( LogSettingsMAwait::IsApplicable(mutation("updateLogLevel")) );
		EXPECT_FALSE( LogSettingsMAwait::IsApplicable(mutation("updateUser")) );
	}
}
