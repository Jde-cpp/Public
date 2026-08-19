#pragma once

namespace Jde::DB::Sqlite::Tests{
	α DS( str path, bool clear=false, SRCE )ε->sp<IDataSource>;
	namespace Schema{
		α Create( str path )ε->void;
		α Resync( str cluster )ε->void; //a second sync of an already-created cluster - what a service restart does.
	}

	//Backend-parameterized fixture ("memory"/"file"): syncs each backend's schema once per process, then hands out the DS()-cached data source.
	struct BackendTests : ::testing::TestWithParam<string>{
		α SetUp()ε->void override;
		sp<IDataSource> _ds;
	};
	#define INSTANTIATE_BACKENDS( suite ) INSTANTIATE_TEST_SUITE_P( Backends, suite, ::testing::Values("memory","file"), []( const auto& info ){ return info.param; } )
}