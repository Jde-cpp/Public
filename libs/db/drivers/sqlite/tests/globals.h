#pragma once

namespace Jde::DB::Sqlite::Tests{
	Ξ FileParamPath()->string{ return (fs::current_path()/("sqlite-tests.db")).string(); }
	α DS( str path, bool clear=false, SRCE )ε->sp<IDataSource>;
	namespace Schema{
		α Create( str path )ε->void;
	}
}