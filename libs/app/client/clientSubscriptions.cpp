#include <jde/app/client/clientSubscriptions.h>
#include <jde/ql/types/Subscription.h>

#define let const auto
namespace Jde::App::Client{
	flat_map<QL::SubscriptionId,flat_set<sp<QL::IListener>>> _subs; std::shared_mutex _mutex;
	α Subscriptions::StopListenRemote( sp<QL::IListener> listener, vector<QL::SubscriptionId> /*ids*/ )ι->jarray{
		jarray y;
		ul _{ _mutex };
		for( auto idListeners = _subs.begin(); idListeners!=_subs.end(); ){
			idListeners->second.erase( listener );
			idListeners = idListeners->second.empty() ? _subs.erase( idListeners ) : next(idListeners);
		}
		return y;
	}
	α Subscriptions::ListenRemote( sp<QL::IListener> listener, QL::Subscription&& sub )ι->void{
		ul _{ _mutex };
		_subs.try_emplace( sub.Id ).first->second.emplace( listener );
	}
	α Subscriptions::OnTraces( App::Proto::FromServer::Traces&& traces, QL::SubscriptionId requestId )ι->void{
		sl l{ _mutex };
		if( let& listener = _subs[requestId].begin(); listener!=_subs.at(requestId).end() )
			(*listener)->OnTraces( move(traces) );
	}
	α Subscriptions::OnWebsocketReceive( const jobject& m, QL::SubscriptionId clientId )ι->void{
		sl l{ _mutex };
		if( auto kv = _subs.find( clientId ); kv!=_subs.end() ){
			for( let& listener : kv->second ){
				try{
					listener->OnChange( m, clientId );
				}
				catch( exception& )
				{}
			}
		}
		else
			WARNT( ELogTags::QL, "[{}]Could not find subscription.", clientId );
	}
}