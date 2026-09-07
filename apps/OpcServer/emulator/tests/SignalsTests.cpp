//Signals:  TagSpec parsing - defaults, the refusals (#9 among them) - and the six generators' shapes.
#include "../Signals.h"

#define let const auto
namespace Jde::Opc::Emulator::Tests{
	Ω spec( sv json )ε->TagSpec{ return TagSpec{ boost::json::parse(string{json}).as_object() }; }
	Ω gen( sv json )ε->up<IGenerator>{ return MakeGenerator( spec(json) ); }

	TEST( ParseModeTests, EveryNameRoundTripsAndNothingElseParses ){
		for( let name : {"sine", "ramp", "randomWalk", "counter", "toggle", "command", "follow"} )
			EXPECT_EQ( ToString(ParseMode(name)), name );
		EXPECT_THROW( ParseMode("triangle"), Exception );
	}

	TEST( TagSpecTests, DefaultsAreTheDocumentedOnes ){
		let s = spec( R"({"name":"x"})" );
		EXPECT_EQ( s.Name, "x" );
		EXPECT_EQ( s.Mode, EMode::Counter );
		EXPECT_EQ( s.Min, 0 ); EXPECT_EQ( s.Max, 100 ); EXPECT_EQ( s.Step, 1 ); EXPECT_EQ( s.RatedRpm, 1450 );
		EXPECT_EQ( s.Period, Duration{30s} ); EXPECT_EQ( s.Tau, Duration{3s} );
		EXPECT_FALSE( s.IsBool() );
	}
	TEST( TagSpecTests, OnlyToggleAndCommandAreBool ){
		EXPECT_TRUE( spec(R"({"name":"x","mode":"toggle"})").IsBool() );
		EXPECT_TRUE( spec(R"({"name":"x","mode":"command"})").IsBool() );
		EXPECT_FALSE( spec(R"({"name":"x","mode":"sine"})").IsBool() );
		EXPECT_FALSE( spec(R"({"name":"x","mode":"follow"})").IsBool() );
	}
	TEST( TagSpecTests, MaxMustExceedMinUnlessTheRangeIsUnused ){
		EXPECT_THROW( spec(R"({"name":"x","mode":"counter","min":10,"max":10})"), Exception );
		EXPECT_THROW( spec(R"({"name":"x","mode":"sine","min":10,"max":5})"), Exception );
		EXPECT_NO_THROW( spec(R"({"name":"x","mode":"toggle","min":10,"max":10})") );//a bool has no range
		EXPECT_NO_THROW( spec(R"({"name":"x","mode":"command","min":10,"max":10})") );
		EXPECT_NO_THROW( spec(R"({"name":"x","mode":"follow","min":10,"max":10})") );//follow tracks ratedRpm, not min/max
	}
	TEST( TagSpecTests, PeriodAndTauMustBePositive ){
		EXPECT_THROW( spec(R"({"name":"x","mode":"sine","period":"PT0S"})"), Exception );
		EXPECT_THROW( spec(R"({"name":"x","mode":"follow","tau":"PT0S"})"), Exception );
	}
	//#9:  randomWalk builds uniform_real_distribution{-step,step} (UB unless -step<=step) and counter never wraps on a
	//non-positive step - both must fail like the config typos they are;  a mode that never reads `step` is not policed.
	TEST( TagSpecTests, StepMustBePositiveWhereItIsUsed ){
		EXPECT_THROW( spec(R"({"name":"x","mode":"randomWalk","step":-10})"), Exception );
		EXPECT_THROW( spec(R"({"name":"x","mode":"randomWalk","step":0})"), Exception );
		EXPECT_THROW( spec(R"({"name":"x","mode":"counter","step":0})"), Exception );
		EXPECT_NO_THROW( spec(R"({"name":"x","mode":"sine","step":-1})") );
	}

	TEST( GeneratorTests, SineSpansMinToMaxOverAPeriod ){
		let g = gen( R"({"name":"s","mode":"sine","min":0,"max":100,"period":"PT8S"})" );
		EXPECT_NEAR( g->Next(2s, false), 100, 1e-9 );//a quarter period: the peak
		EXPECT_NEAR( g->Next(2s, false), 50, 1e-9 );//half: back through the middle
		EXPECT_NEAR( g->Next(2s, false), 0, 1e-9 );//three quarters: the trough
		EXPECT_NEAR( g->Next(2s, false), 50, 1e-9 );
	}
	TEST( GeneratorTests, RampIsASawtoothThatWraps ){
		let g = gen( R"({"name":"r","mode":"ramp","min":0,"max":100,"period":"PT10S"})" );
		EXPECT_NEAR( g->Next(2500ms, false), 25, 1e-9 );
		EXPECT_NEAR( g->Next(5s, false), 75, 1e-9 );
		EXPECT_NEAR( g->Next(5s, false), 25, 1e-9 );//12.5 s into a 10 s period
	}
	TEST( GeneratorTests, RandomWalkStaysInBoundsAndNeverJumpsMoreThanStep ){
		let g = gen( R"({"name":"w","mode":"randomWalk","min":40,"max":60,"step":5})" );
		double previous{ 50 };//starts at the middle
		for( uint i=0; i<2000; ++i ){
			let v = g->Next( 1s, false );
			ASSERT_GE( v, 40 ); ASSERT_LE( v, 60 );
			ASSERT_LE( std::abs(v-previous), 5+1e-9 );
			previous = v;
		}
	}
	TEST( GeneratorTests, CounterStepsAndWrapsToMin ){
		let g = gen( R"({"name":"c","mode":"counter","min":0,"max":3,"step":1})" );
		EXPECT_EQ( g->Next(1s, false), 1 );
		EXPECT_EQ( g->Next(1s, false), 2 );
		EXPECT_EQ( g->Next(1s, false), 3 );
		EXPECT_EQ( g->Next(1s, false), 0 );//4 > max wraps
		EXPECT_EQ( g->Next(1s, false), 1 );
	}
	TEST( GeneratorTests, ToggleStartsOnAndFlipsEveryPeriod ){
		let g = gen( R"({"name":"t","mode":"toggle","period":"PT2S"})" );
		EXPECT_EQ( g->Next(1s, false), 1 );
		EXPECT_EQ( g->Next(1s, false), 0 );//t reaches the period
		EXPECT_EQ( g->Next(2s, false), 1 );
	}
	TEST( GeneratorTests, FollowLagsTowardsRatedWhenCommandedAndTowardsZeroWhenNot ){
		let g = gen( R"({"name":"f","mode":"follow","ratedRpm":1450,"tau":"PT1S"})" );
		EXPECT_NEAR( g->Next(1s, true), 1450*(1-std::exp(-1.0)), 1e-6 );//916.6 - the emulator's first status line
		let second = g->Next( 1s, true );
		EXPECT_GT( second, 916 ); EXPECT_LT( second, 1450 );
		EXPECT_NEAR( g->Next(30s, true), 1450, 1e-6 );//settled
		EXPECT_NEAR( g->Next(1s, false), 1450*std::exp(-1.0), 1e-6 );//commanded off: decays
	}
	TEST( GeneratorTests, CommandTagsHaveNoGenerator ){
		EXPECT_THROW( gen(R"({"name":"status","mode":"command"})"), Exception );
	}
}
