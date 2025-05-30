#pragma once
#include <jde/ql/usings.h>
#include "accessInternal.h"

namespace Jde::QL{ struct IQL; }
namespace Jde::Access{
	struct AccessListener : QL::IListener, IShutdown{
		AccessListener( sp<QL::IQL> qlServer )ι:QL::IListener{"Access"},_qlServer{qlServer}{}
		Ω Instance()ι->sp<AccessListener>;
		Ω SetInstance( sp<QL::IQL> qlServer )ι->void;
		α OnChange( const jvalue& j, QL::SubscriptionId clientId )ε->void override;
		α Shutdown( bool /*terminate*/ )ι->void;
		sp<QL::IQL> _qlServer;
	private:
		α AclChanged( ESubscription event, const jobject& o )ι->void;
		α UserChanged( UserPK userPK, ESubscription event, const jobject& o )ι->void;
		α GroupChanged( GroupPK groupPK, ESubscription event, const jobject& o )ε->void;
		α RoleChanged( RolePK rolePK, ESubscription event, const jobject& o )ε->void;
		α ResourceChanged( ResourcePK resourcePK, ESubscription event, const jobject& o )ε->void;
		α PermissionUpdated( PermissionRightsPK pk, const jobject& o )ε->void;
	};
}