#include <jde/access/awaits/EventsSubscribeAwait.h>
#include <jde/db/names.h>
#include <jde/ql/IQL.h>
#include <jde/access/AccessListener.h>
#include "../accessInternal.h"


#define let const auto
namespace Jde::Access{
	constexpr sv format{ "subscription {0}{2}{{ {1}{2}(subscriptionId:$id){{{3}}} }}" }; //subscription UserCreated{ userCreated(subscriptionId:129){id} }
	α EventTypeSubscribeAwait::Subscribe()ι->TAwait<vector<QL::SubscriptionId>>::Task{
		using enum ESubscription;
		let capitalized = DB::Names::Capitalize( _name );
		auto vars = [&]( ESubscription event )->jobject {
			jobject vars = _vars;
			vars["id"] = underlying( _type | event );
			return vars;
		};
		try{
			if( !empty(_events & Created) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Created", _cols), vars(Created), _listener, _executer );
			if( !empty(_events & Deleted) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Deleted", _cols), vars(Deleted), _listener, _executer );
			if( !empty(_events & Restored) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Restored", _cols), vars(Restored), _listener, _executer );
			if( !empty(_events & Purged) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Purged", _cols), vars(Purged), _listener, _executer );
			if( !empty(_events & Added) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Added", _cols), vars(Added), _listener, _executer );
			if( !empty(_events & Removed) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Removed", _cols), vars(Removed), _listener, _executer );
			if( !empty(_events & Updated) )
				co_await *_qlServer->Subscribe( Ƒ(format, capitalized, _name, "Updated", _cols), vars(Updated), _listener, _executer );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}

	α EventsSubscribeAwait::Execute()ι->EventTypeSubscribeAwait::Task{
		using enum ESubscription;
		jobject vars{ {"schemas", boost::json::value_from(_schemas)} };
		try{
			co_await EventTypeSubscribeAwait{ _qlServer, "user", User, "id", Created | Deleted | Restored | Purged, {}, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "grouping", Group, "id", Deleted | Restored | Purged, {}, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "grouping", Group, "id memberId", Added | Removed, {}, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "role", Role, "id", Deleted | Restored | Purged, {}, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "role", Role, "id permissionRight{id allowed denied resource(schemaName:$schemas){id target schemaName target criteria}} role{id}", Added | Removed, vars, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "resources", Resources, "(schema:$schemas)id schemaName target criteria deleted", Created, vars, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "resources", Resources, "(schema:$schemas)id schema target", Deleted | Restored, vars, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "permission", Permission, "id allowed denied resource(schema:$schemas)", Updated, vars, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "acl", Acl, "identity{id} permissionRight{id allowed denied resource(schema:$schemas){id}} role{id}", Created, vars, _executer, _listener };
			co_await EventTypeSubscribeAwait{ _qlServer, "acl", Acl, "identity{id} permissionRight{id resource(schema:$schemas)} role{id}", Purged, vars, _executer, _listener };
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}