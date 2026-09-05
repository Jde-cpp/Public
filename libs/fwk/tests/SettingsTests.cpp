#include <jde/fwk/settings.h>
#include <jde/fwk/process/process.h>
#include <cstdlib>

#define let const auto

namespace Jde::Tests{
	//Args() feeds -settings/-include/-tests, so every suite's config selection goes through these rules and none of
	//them was asserted anywhere.  ParseArgs is the seam: Args() itself can only ever see this process's own argv,
	//and the rules that matter are the ones it applies to tokens the platform already split.
	Ω parsed( const vector<string>& tokens )ι->flat_multimap<string,string>{ return Process::ParseArgs( tokens ); }
	Ω one( const flat_multimap<string,string>& args, str key )ι->optional<string>{
		let p = args.find( key );
		return p!=args.end() ? p->second : optional<string>{};
	}

	TEST( ProcessTests, ParseArgsBindsEachForm ){
		let args = parsed( {"exe", "-settings=/a/b.jsonnet", "-tests", "-include", "args/sqlite", "positional", "-arg", "path=:memory:"} );
		EXPECT_EQ( one(args,"-settings"), "/a/b.jsonnet" ) << "-k=v splits on the first '='";
		EXPECT_EQ( one(args,"-tests"), "" ) << "a flag with a flag after it binds empty, and must still be *present*";
		EXPECT_TRUE( args.contains("-tests") ) << "an empty value is not absence - FindArg('-tests') gates the whole test-mode path";
		EXPECT_EQ( one(args,"-include"), "args/sqlite" ) << "-k v binds on the next token";
		EXPECT_EQ( one(args,"-arg"), "path=:memory:" ) << "only the *flag* splits on '=' - a value keeps its own";
		//non-flag tokens with no flag pending are positional, under the empty key.  argv[0] is one of them, which is
		//what Process::Executable() reads on windows.
		let positionals = args.equal_range( string{} );
		EXPECT_EQ( std::distance(positionals.first, positionals.second), 2 );
		EXPECT_EQ( positionals.first->second, "exe" ) << "argv[0] is the first positional";
	}

	TEST( ProcessTests, ParseArgsStripsQuotesAndHandlesEdges ){
		//lldb and VS Code pass argv unshelled, so the quotes arrive as literal characters.
		EXPECT_EQ( one(parsed({"-settings=\"/a b/c.jsonnet\""}), "-settings"), "/a b/c.jsonnet" );
		EXPECT_EQ( one(parsed({"-k=\"unclosed"}), "-k"), "\"unclosed" ) << "only a matched pair is stripped";
		EXPECT_EQ( one(parsed({"-k=\"\""}), "-k"), "" ) << "an empty quoted value is empty, not a lone quote";
		EXPECT_EQ( one(parsed({"-k="}), "-k"), "" ) << "a trailing '=' is an empty value";
		EXPECT_EQ( one(parsed({"-k=a=b"}), "-k"), "a=b" ) << "the *first* '=' splits - a value may contain more, as -arg path=:memory: does";
		EXPECT_EQ( one(parsed({"-a", "-b", "-c"}), "-a"), "" ) << "every flag in a run of flags is kept";
		EXPECT_TRUE( parsed({"-a","-b","-c"}).contains("-c") ) << "including the last, which only the post-loop flush emplaces";
		EXPECT_EQ( parsed({}).size(), 0u );
		//a repeated flag keeps both: it is a multimap, and -arg is passed more than once by the ctest registration.
		let repeated = parsed( {"-arg", "a=1", "-arg", "b=2"} );
		EXPECT_EQ( repeated.count("-arg"), 2u );
	}

	//and the parsed rules really are what this process is running under - the two forms every suite passes.
	TEST( ProcessTests, ThisSuiteArgsParsed ){
		let testFlag = Process::FindArg("-tests") ? Process::FindArg("-tests") : Process::FindArg("-ctest");//direct runs pass -tests, addJdeTest passes -ctest; settings.cpp binds the ext vars on either.
		ASSERT_TRUE( testFlag.has_value() ) << "-tests/-ctest is how the config binds its ext vars; without one settings evaluation fails";
		EXPECT_EQ( *testFlag, "" );
		let settings = Process::FindArg( "-settings" );
		ASSERT_TRUE( settings.has_value() );
		EXPECT_TRUE( settings->ends_with("Framework.Tests.jsonnet") ) << *settings;
	}

	Ω setEnv( const char* name, const char* value )ι->void{
#ifdef _WIN32
		::_putenv_s( name, value );//what GetEnv's _dupenv_s reads.
#else
		::setenv( name, value, 1 );
#endif
	}
	//Settings::Value() is a jvalue, so every lookup is a json pointer and try_at_pointer rejects one with no leading
	//'/' - a path spelled "cryptoTests/clear" resolves to nothing and the caller's value_or() default wins silently.
	//That is how /http/accessControl/allowOrigin, /server/host, /web/client/timeout and /cryptoTests/clear all sat
	//dead for months.  These pin both halves: the rooted spelling reads the config, the unrooted one reads nothing.
	struct SettingsTests : public ::testing::Test{};

	TEST_F( SettingsTests, RootedPathResolves ){
		EXPECT_EQ( Settings::FindBool("/cryptoTests/clear"), false );//not just has_value - the config sets it to the opposite of every caller's default, so a silent miss is indistinguishable from a hit unless the value is asserted.
		EXPECT_EQ( Settings::FindString("/logging/spd/tags/default"), "Information" );
		EXPECT_EQ( Settings::FindNumber<uint16>("/workers/executor/threads"), 2 );
		EXPECT_TRUE( Settings::FindPath("/testing/file").has_value() );
		EXPECT_NE( Settings::FindObject("/logging/spd"), nullptr );
	}

	//The trap, pinned deliberately: a Find* that comes back empty for a key plainly present in the config means the
	//path is missing its leading '/'.  If tolerance is ever added to Settings, this test is what says so.
	TEST_F( SettingsTests, UnrootedPathResolvesToNothing ){
		EXPECT_FALSE( Settings::FindBool("cryptoTests/clear").has_value() );
		EXPECT_FALSE( Settings::FindString("logging/spd/tags/default").has_value() );
		EXPECT_FALSE( Settings::FindNumber<uint16>("workers/executor/threads").has_value() );
		EXPECT_EQ( Settings::FindObject("logging/spd"), nullptr );
	}

	//a rooted path that matches nothing is still empty - no partial-match fallback.
	TEST_F( SettingsTests, MissingPathIsEmpty ){
		EXPECT_FALSE( Settings::FindBool("/cryptoTests/notAKnob").has_value() );
		EXPECT_FALSE( Settings::FindString("/notASection/notAKnob").has_value() );
	}

	//Set's create_objects replaced a hand-rolled fallback that corrupted the root, so the unrelated key below is
	//half the point of the test.  Everything here writes under /settingsTests, which nothing else reads.
	TEST_F( SettingsTests, SetCreatesIntermediateObjects ){
		Settings::Set( "/settingsTests/nested/value", 42 );
		EXPECT_EQ( Settings::FindNumber<uint>("/settingsTests/nested/value"), 42u );
		EXPECT_NE( Settings::FindObject("/settingsTests/nested"), nullptr ) << "the intermediate object must exist, not just the leaf";
		EXPECT_EQ( Settings::FindBool("/cryptoTests/clear"), false ) << "an unrelated key must survive the set";

		Settings::Set( "/settingsTests/nested/value", 43 );//and an existing leaf overwrites in place.
		EXPECT_EQ( Settings::FindNumber<uint>("/settingsTests/nested/value"), 43u );
	}

	//$(NAME) expansion has produced silent production misconfigurations twice - HostName resolving to "" before it
	//was a builtIn, and array entries left literal - and had no test either time.
	TEST_F( SettingsTests, EnvExpansion ){
		setEnv( "JDE_TEST_ENV", "expanded" );
		Settings::Set( "/settingsTests/env", "[$(JDE_TEST_ENV)][$(HostName)][$(JDE_NOT_SET_12345)]" );
		EXPECT_EQ( Settings::FindString("/settingsTests/env"), Ƒ("[expanded][{}][]", Process::HostName()) ) << "env var, builtIn, and unknown -> empty";

		//a real environment variable outranks the builtIn of the same name.
		let hostName = Process::GetEnv( "HostName" );
		setEnv( "HostName", "overridden-host" );
		Settings::Set( "/settingsTests/host", "$(HostName)" );
		EXPECT_EQ( Settings::FindString("/settingsTests/host"), "overridden-host" );
		setEnv( "HostName", hostName ? hostName->c_str() : "" );//"" reads back as unset, which is where it started.
		EXPECT_EQ( Settings::FindString("/settingsTests/host"), Process::HostName() );
	}

	//a value naming itself rescans forever without the 32-pass bound.  Note the failure mode this guards is a hang,
	//so removing the bound would wedge this test rather than fail it.
	TEST_F( SettingsTests, CyclicExpansionTerminates ){
		setEnv( "JDE_TEST_CYCLE", "$(JDE_TEST_CYCLE)" );
		Settings::Set( "/settingsTests/cycle", "$(JDE_TEST_CYCLE)" );
		EXPECT_EQ( Settings::FindString("/settingsTests/cycle"), "$(JDE_TEST_CYCLE)" ) << "gives up unexpanded after the bound, with one WARN";
	}

	//expanding only objects at load left every trustedCertDirs/scriptPaths entry literal unless its reader happened
	//to be Find{String,Path}Array - which do expand, and are what this pins.
	TEST_F( SettingsTests, ArrayEntriesExpand ){
		setEnv( "JDE_TEST_DIR", "/jde-test-dir" );
		Settings::Set( "/settingsTests/dirs", jarray{"$(JDE_TEST_DIR)/a", "$(JDE_TEST_DIR)/b", 7} );
		let strings = Settings::FindStringArray( "/settingsTests/dirs" );
		ASSERT_EQ( strings.size(), 2u ) << "non-string entries are skipped, not stringified";
		EXPECT_EQ( strings[0], "/jde-test-dir/a" );
		let paths = Settings::FindPathArray( "/settingsTests/dirs" );
		ASSERT_EQ( paths.size(), 2u );
		EXPECT_EQ( paths[1], fs::path{"/jde-test-dir/b"} );
		EXPECT_TRUE( Settings::FindStringArray("/settingsTests/notAnArray").empty() );
	}

	TEST_F( SettingsTests, FindDuration ){
		Settings::Set( "/settingsTests/duration", "PT1M" );
		EXPECT_EQ( Settings::FindDuration("/settingsTests/duration"), 1min );
		EXPECT_FALSE( Settings::FindDuration("/settingsTests/notADuration").has_value() );
	}
}
