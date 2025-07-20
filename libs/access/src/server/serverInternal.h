#pragma once

namespace Jde::DB{ struct AppSchema; struct IDataSource; struct View; }
namespace QL{ struct LocalQL; }
namespace Jde::Access{ struct Authorize; }
namespace Jde::Access::Server{
	α LocalQL()ι->QL::LocalQL&;
	α Authorizer()ι->Authorize&;
	α DS()ι->DB::IDataSource&;
	α GetTablePtr( str name, SRCE )ε->sp<DB::View>;
	α GetTable( str name, SRCE )ε->const DB::View&;
	α AccessSchema()ι->DB::AppSchema&;
//	α GetSchemaPtr()ι->sp<DB::AppSchema>; accessInternal.h
//	α SetSchema( sp<DB::AppSchema> schema )ι->void{ /*ASSERT(!_schema);*/ _schema = schema; } accessInternal.h
}