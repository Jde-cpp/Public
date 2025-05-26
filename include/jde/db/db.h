#pragma once
#include <jde/framework/io/json.h>
#include <jde/db/exports.h>

#define Φ ΓDB α

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	struct Cluster; struct IDataSource; struct AppSchema;

	α DataSource( const fs::path& libraryName, sv connectionString )ε->sp<IDataSource>;
	Φ GetAppSchema( str name, sp<Access::IAcl> )ε->sp<AppSchema>;
	Φ SyncSchema( AppSchema& schema, sp<QL::IQL> ql )ε->void;
#ifndef PROD
	namespace NonProd{
		Φ Recreate( const AppSchema& schema, sp<QL::IQL> ql )ε->void;
	}
#endif
}
#undef Φ