#pragma once

namespace Jde::DB{ struct AppSchema; struct IDataSource; struct View; }
namespace Jde::Opc::Gateway{
	α DS()ι->sp<DB::IDataSource>;
	α SetSchema( sp<DB::AppSchema> schema )ι->void;
	α GetViewPtr( str name )ι->sp<DB::View>;
}