#pragma once
#include <jde/access/usings.h>

namespace Jde::DB{ struct AppSchema; struct IDataSource; struct View; }
namespace Jde::Access{
	struct Authorize;

	α GetTable( str name )ε->sp<DB::View>;
	α GetSchema()ι->sp<DB::AppSchema>;
	α SetSchema( sp<DB::AppSchema> schema )ι->void;
	α DS()ι->sp<DB::IDataSource>;
	α Authorizer()ι->Authorize&;
	α AuthorizerPtr()ι->sp<Authorize>;
}