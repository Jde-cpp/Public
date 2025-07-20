#include "accessInternal.h"
#include <jde/access/usings.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/access/Authorize.h>

namespace Jde{
	static sp<DB::AppSchema> _schema;
	α Access::GetSchemaPtr()ι->sp<DB::AppSchema>{ ASSERT(_schema); return _schema; }
	α Access::GetSchema()ι->DB::AppSchema&{ return *GetSchemaPtr(); }
	α Access::SetSchema( sp<DB::AppSchema> schema )ι->void{ /*ASSERT(!_schema);*/ _schema = schema; }
}