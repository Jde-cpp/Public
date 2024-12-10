#pragma once
#include <jde/framework/io/json.h>

namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	struct Cluster; struct IDataSource; struct AppSchema;

	α DataSource( const fs::path& libraryName, sv connectionString )ε->sp<IDataSource>;
	α GetAppSchema( str name, sp<Access::IAcl> )ε->sp<AppSchema>;
	α SyncSchema( AppSchema& schema )ε->void;
#ifndef PROD
	namespace NonProd{
		α Recreate( const AppSchema& schema )ε->void;
	}
#endif
}
