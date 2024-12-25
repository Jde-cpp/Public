#include <jde/access/access.h>
#include <jde/access/IAcl.h>
#include "hooks/RoleHook.h"
#include "hooks/GroupHook.h"
#include <boost/container/flat_set.hpp>
#include <jde/framework/str.h>
#include <jde/framework/io/file.h>
#include <jde/db/IDataSource.h>
#include <jde/db/generators/InsertClause.h>
#include "hooks/AclHook.h"
#include "awaits/ResourceLoadAwait.h"
#include <jde/access/types/Group.h>
#include <jde/access/types/Role.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include "../../../../Framework/source/DateTime.h"
#include "accessInternal.h"
#include <jde/access/Authorize.h>
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

	α Access::Configure( sp<DB::AppSchema> access, vector<string> schemaNames, sp<QL::IQL> qlServer, UserPK executer )ε->ConfigureAwait{
		SetSchema( access );
		Resources::Sync();
		if( access ){
			QL::Hook::Add( mu<Access::AclHook>() );
			QL::Hook::Add( mu<Access::UserHook>() );
			QL::Hook::Add( mu<Access::GroupHook>() );
			QL::Hook::Add( mu<Access::RoleHook>() );
		}
		return {qlServer, schemaNames, executer};
	}
	α Access::Authenticate( str loginName, uint providerId, str opcServer, SL sl )ι->AuthenticateAwait{
		return AuthenticateAwait{ loginName, providerId, opcServer, sl };
	}
}