#include "jde/fwk/io/Cache.h"
#include "jde/fwk/chrono.h"
#include <string>

#define let const auto
namespace Jde::Tests{
	//constexpr ELogTags _tags{ ELogTags::Test };
	struct CacheTests : public ::testing::Test
	{};

	TEST_F( CacheTests, NegativeDuration ){
		Cache::Set<string>( "negative", "prior" );//negative-duration Set must also drop an existing entry, not just skip caching.
		auto p = Cache::Set<string>( "negative", "value", -1s );
		//ASSERT_FALSE( Cache::Has("negative") );
		ASSERT_EQ( Cache::Get<string>("negative"), nullptr );
		ASSERT_TRUE( p );
		ASSERT_EQ( *p, "value" );
	}
	TEST_F( CacheTests, Expires ){
		auto p = Cache::Set<string>( "expired", "value", 1ms );
		std::this_thread::sleep_for( 2ms );
		ASSERT_EQ( Cache::Get<string>("expired"), nullptr );
	}
	TEST_F( CacheTests, Stress ){
		let threadCount = std::max( 2u, std::thread::hardware_concurrency() );
		let end = steady_clock::now()+1s;
		vector<string> keys;
		for( uint i=0; i<5; ++i )
			keys.push_back( Ƒ("{}", i) );
		std::atomic<uint> retrieved{}, missed{}, sets{};
		{
			vector<std::jthread> workers;
			for( uint t=0; t<threadCount; ++t ){
				workers.emplace_back( [&, t]()ι->void{
					uint localRetrieved{}, localMissed{}, localSets{};
					for( uint i=t; steady_clock::now()<end; ++i ){
						let& key = keys[i%keys.size()];
						if( Cache::Get<string>(key) )
							++localRetrieved;
						else
							++localMissed;
						if( i%7==0 )
							Cache::Clear( key );
						else{
							Cache::Set<string>( key, ToIsoString(steady_clock::now()), i%3==0 ? 1ms : 10ms );
							++localSets;
						}
					}
					retrieved += localRetrieved; missed += localMissed; sets += localSets;
				} );
			}
		}//jthreads join here.
		INFOT( ELogTags::Test, "threads: {}, retrieved: {}, missed: {}, sets: {}", threadCount, retrieved.load(), missed.load(), sets.load() );
		std::this_thread::sleep_for( 11ms );
		for( let& key : keys )
			ASSERT_EQ( Cache::Get<string>(key), nullptr );
	}
}