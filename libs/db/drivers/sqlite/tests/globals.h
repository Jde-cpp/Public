#pragma once

namespace Jde::DB::Sqlite::Tests{
	α DS( str path, bool clear=false, SRCE )ε->sp<IDataSource>;
	namespace Schema{
		α Create( str path )ε->void;
	}

	//Backend-parameterized fixture ("memory"/"file"): syncs each backend's schema once per process, then hands out the DS()-cached data source.
	struct BackendTests : ::testing::TestWithParam<string>{
		α SetUp()ε->void override{
			const auto& cluster = GetParam();
			static flat_set<string> synced;
			if( synced.insert(cluster).second )
				Schema::Create( cluster );
			_ds = DS( cluster );
		}
		sp<IDataSource> _ds;
	};
	#define INSTANTIATE_BACKENDS( suite ) INSTANTIATE_TEST_SUITE_P( Backends, suite, ::testing::Values("memory","file"), []( const auto& info ){ return info.param; } )
}