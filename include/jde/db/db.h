#pragma once
#include <jde/framework/io/json.h>

namespace Jde::DB{
	struct Cluster; struct Schema;

	α GetSchema( sv name )ε->sp<Schema>;
	α SyncSchema( sv name )ε->sp<Schema>;
#ifndef PROD
	namespace NonProd{
		α Recreate( sp<Schema>& schema )ε->void;
	}
#endif
}
