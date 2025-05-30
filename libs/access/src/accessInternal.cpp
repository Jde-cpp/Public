#include "accessInternal.h"
#include <jde/access/usings.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/access/Authorize.h>

namespace Jde{
	static sp<DB::AppSchema> _schema;
	sp<Access::Authorize> _authorizer = ms<Access::Authorize>();

	α Access::Authorizer()ι->Authorize&{ return *_authorizer; }
	α Access::AuthorizerPtr()ι->sp<Authorize>{ return _authorizer;}

	α Access::GetTable( str name )ε->sp<DB::View>{ ASSERT(_schema); return _schema->GetViewPtr( name ); }
	α Access::GetSchema()ι->sp<DB::AppSchema>{ ASSERT(_schema); return _schema; }
	α Access::SetSchema( sp<DB::AppSchema> schema )ι->void{ ASSERT(!_schema); _schema = schema; }

	α Access::DS()ι->sp<DB::IDataSource>{ ASSERT(_schema); return _schema->DS(); }
}