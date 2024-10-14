#pragma once
#include <jde/access/usings.h>
#include "Permission.h"

namespace Jde::DB{ struct Schema; }
namespace Jde::Access{

	struct AclLoadAwait final : TAwait<flat_multimap<IdentityPK,PermissionPK>>{
		AclLoadAwait( sp<DB::Schema> schema )ι: _schema{schema}{};
	private:
		α Suspend()ι->void override;
		sp<DB::Schema> _schema;
	};
}