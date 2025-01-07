//#include <jde/ql/SubscriptionAwait.h>
#include <jde/ql/IQL.h>
#include <jde/ql/QLSubscriptions.h>


#define let const auto
namespace Jde::QL{
	α SubscriptionAwait::Execute()ι->TAwait<vector<SubscriptionId>>::Task{
		try{
			auto subscriptionIds = co_await *_qlServer->Subscribe( move(_query), _clientId, _executer, _sl );
			for( let& subscriptionId : subscriptionIds ){
				let serverId = Json::AsNumber<SubscriptionId>(subscriptionId);
				_listener->Ids.push_back( serverId );
				Subscriptions::Listen( _listener, serverId, _clientId );
			}
			Resume( move(subscriptionIds) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
	α UnsubscribeAwait::Execute()ι->TAwait<jvalue>::Task{
		try{
			co_await *_qlServer->Query( Ƒ("unsubscribe( id:[{}] )", Str::Join(_ids, ",")), {0}, _sl );
			Resume();
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}