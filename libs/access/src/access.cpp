#include <jde/access/access.h>
#include <boost/container/flat_set.hpp>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include "../../../../Framework/source/DateTime.h"
#include <jde/access/Authorize.h>
#include <jde/access/IAcl.h>
#include <jde/access/types/Group.h>
#include <jde/access/types/Role.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include "accessInternal.h"
#include "awaits/ResourceLoadAwait.h"
#include "hooks/AclHook.h"
#include "hooks/GroupHook.h"
#include "hooks/RoleHook.h"
#include "hooks/UserHook.h"


#define let const auto

namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };
	constexpr array<sv,8> ProviderTypeStrings = { "None", "Google", "Facebook", "Amazon", "Microsoft", "VK", "key", "OpcServer" };
	α ToProviderType( sv x )ι->EProviderType{ return ToEnum<EProviderType>( ProviderTypeStrings, x ).value_or(EProviderType::None); }
	α ToString( EProviderType x )ι->sv{ return FromEnum<EProviderType>( ProviderTypeStrings, x ); }
}

namespace Jde{
	α Access::LocalAcl()ι->sp<IAcl>{
		return AuthorizerPtr();
	}

	α Access::Configure( sp<DB::AppSchema> access, vector<sp<DB::AppSchema>>&& schemas, sp<QL::IQL> qlServer, UserPK executer )ε->ConfigureAwait{
		SetSchema( access );
		Resources::Sync( schemas, qlServer, executer );
		if( access ){
			QL::Hook::Add( mu<Access::AclHook>() );//select, insertBefore
			QL::Hook::Add( mu<Access::GroupHook>() );//add before
			QL::Hook::Add( mu<Access::RoleHook>() );//add remove
			QL::Hook::Add( mu<Access::UserHook>() );//select
		}
		return { qlServer, move(schemas), executer };
	}
	α Access::Authenticate( str loginName, uint providerId, str opcServer, SL sl )ι->AuthenticateAwait{
		return AuthenticateAwait{ loginName, providerId, opcServer, sl };
	}
}