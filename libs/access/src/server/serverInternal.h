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
}