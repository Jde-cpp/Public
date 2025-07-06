//#include <jde/access/access.h>
#include <boost/container/flat_set.hpp>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/framework/chrono.h>
#include <jde/ql/IQL.h>
#include <jde/access/Authorize.h>
#include <jde/access/IAcl.h>
#include <jde/access/types/Group.h>
#include <jde/access/types/Role.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include "accessInternal.h"
#include "awaits/ResourceLoadAwait.h"


#define let const auto

namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };
	α ToProviderType( sv x )ι->EProviderType{ return ToEnum<EProviderType>( ProviderTypeStrings, x ).value_or(EProviderType::None); }
	α ToString( EProviderType x )ι->sv{ return FromEnum<EProviderType>( ProviderTypeStrings, x ); }
}