#include "opcInternal.h"

#define let const auto

namespace Jde{
	sp<DB::AppSchema> _schema;
	α Opc::DS()ι->sp<DB::IDataSource>{ return _schema->DS(); }
	α Opc::SetSchema( sp<DB::AppSchema> schema )ι->void{ _schema = schema; }

	α Opc::GetViewPtr( str name )ι->sp<DB::View>{
		return _schema->GetViewPtr( name );
	}
}