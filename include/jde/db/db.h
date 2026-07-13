#pragma once
#include <jde/fwk/io/json.h>
#include <jde/db/exports.h>

#define Φ ΓDB α

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{ struct IAcl; }
namespace Jde::DB{
	struct Cluster; struct IDataSource; struct AppSchema;

	α DataSource( jobject config, SRCE )ε->sp<IDataSource>;
	Φ GetCluster( sv configName, sp<Access::IAcl>, SRCE )ε->sp<Cluster>; //resolve a specific cluster by its dbServers key (e.g. two backends configured side by side).
	Φ GetAppSchema( str name, sp<Access::IAcl>, optional<jobject> dbSettings=nullopt )ε->sp<AppSchema>;
	Φ SyncSchema( const AppSchema& schema, sp<QL::IQL> ql )ε->void;
#ifndef PROD
	namespace NonProd{
		Φ Recreate( const AppSchema& schema, sp<QL::IQL> ql )ε->void;
	}
#endif
}
#undef Φ