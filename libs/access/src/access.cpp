//#include <jde/access/access.h>
#include <boost/container/flat_set.hpp>
#include <jde/fwk/str.h>
#include <jde/fwk/io/file.h>
#include <jde/fwk/chrono.h>
#include <jde/ql/IQL.h>
#include <jde/access/Authorize.h>
#include <jde/access/IAcl.h>
#include <jde/access/types/Group.h>
#include <jde/access/types/Role.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include "accessInternal.h"

#define let const auto

namespace Jde{
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };
	α /*Access::*/ToProviderType( sv x )ι->Access::EProviderType{ return ToEnum<Access::EProviderType>( ProviderTypeStrings, x ).value_or(Access::EProviderType::None); }
	α /*Access::*/ToString( Access::EProviderType x )ι->string{ return FromEnum<Access::EProviderType>( ProviderTypeStrings, x ); }
}