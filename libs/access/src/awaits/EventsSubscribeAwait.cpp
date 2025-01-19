#include "EventsSubscribeAwait.h"
#include <jde/db/names.h>
#include <jde/ql/IQL.h>
#include "../accessInternal.h"
#include "../AccessListener.h"

#define let const auto
namespace Jde::Access{
	α EventTypeSubscribeAwait::Subscribe()ι->TAwait<vector<QL::SubscriptionId>>::Task{
		using enum ESubscription;
		constexpr sv format{ "subscription {0}{2}{{ {1}{2}(subscriptionId:{3}){{{4}}} }}" };
		let capitalized = DB::Names::Capitalize( _name );
		auto listener = AccessListener::Instance();
		try{
			if( !empty(_events & Created) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Created", underlying(_type | Created), _cols), listener, _executer );
			if( !empty(_events & Deleted) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Deleted", underlying(_type | Deleted), _cols), listener, _executer );
			if( !empty(_events & Restored) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Restored", underlying(_type | Restored), _cols), listener, _executer );
			if( !empty(_events & Purged) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Purged", underlying(_type | Purged), _cols), listener, _executer );
			if( !empty(_events & Added) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Added", underlying(_type | Added), _cols), listener, _executer );
			if( !empty(_events & Removed) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Removed", underlying(_type | Removed), _cols), listener, _executer );
			if( !empty(_events & Updated) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Updated", underlying(_type | Updated), _cols), listener, _executer );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α EventsSubscribeAwait::Execute()ι->EventTypeSubscribeAwait::Task{
		using enum ESubscription;
		AccessListener::SetInstance( _qlServer );
		try{
			co_await EventTypeSubscribeAwait{ _qlServer, "user", User, "id", Created | Deleted | Restored | Purged, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "identityGroup", Group, "id", Deleted | Restored | Purged, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "identityGroup", Group, "id memberId", Added | Removed, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "role", Role, "id", Deleted | Restored | Purged, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "role", Role, "id permissionRight{id allowed denied resource{target}} role{id}", Added | Removed, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "resources", Resources, "id schemaName target criteria deleted", Created, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "resources", Resources, "id schema target", Deleted | Restored, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "permission", Permission, "id allowed denied", Updated, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "acl", Acl, "identity{id} permissionRight{id allowed denied resource{id}} role{id}", Created, _executer, _sl };
			co_await EventTypeSubscribeAwait{ _qlServer, "acl", Acl, "identity{id} permissionRight{id} role{id}", Purged, _executer, _sl };
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}