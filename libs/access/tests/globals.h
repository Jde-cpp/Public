#pragma once

namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::Access::Tests{
	α SetSchema( sp<DB::AppSchema> schema )ι->void;
	α DS()ι->DB::IDataSource&;
	α CreateUser( str name, uint providerId=(uint)Access::EProviderType::Google )ι->UserPK;
	α PurgeUser( UserPK userId )ι->void;
}
