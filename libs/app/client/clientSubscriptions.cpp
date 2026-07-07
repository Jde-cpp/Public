#include <jde/app/client/clientSubscriptions.h>
#include <jde/ql/types/Subscription.h>

#define let const auto
namespace Jde::App::Client{
	flat_map<QL::SubscriptionId,flat_set<sp<QL::IListener>>> _subs; std::shared_mutex _mutex;
	α Subscriptions::Clear()ι->void{
		ul _{ _mutex };
		_subs.clear();
	}
	α Subscriptions::StopListenRemote( sp<QL::IListener> listener, vector<QL::SubscriptionId> ids )ι->flat_set<QL::SubscriptionId>{//returns ids removed - caller unsubscribes server side, e.g. IQL::Unsubscribe.
		flat_set<QL::SubscriptionId> y;
		ul _{ _mutex };
		if( ids.empty() ){//all of listener's subscriptions.
			for( auto idListeners = _subs.begin(); idListeners!=_subs.end(); ){
				if( idListeners->second.erase(listener) )
					y.emplace( idListeners->first );
				idListeners = idListeners->second.empty() ? _subs.erase( idListeners ) : next(idListeners);
			}
		}
		else{
			for( let id : ids ){
				auto kv = _subs.find( id );
				if( kv==_subs.end() || !kv->second.erase(listener) )
					continue;
				y.emplace( id );
				if( kv->second.empty() )
					_subs.erase( kv );
			}
		}
		return y;
	}
	α Subscriptions::ListenRemote( sp<QL::IListener> listener, QL::Subscription&& sub )ι->void{
		ul _{ _mutex };
		_subs.try_emplace( sub.Id ).first->second.emplace( listener );
	}
	α Subscriptions::OnTraces( App::Proto::FromServer::Traces&& traces, QL::SubscriptionId requestId )ι->void{
		sl l{ _mutex };
		if( auto kv = _subs.find( requestId ); kv!=_subs.end() ){
			for( auto listener = kv->second.begin(); listener!=kv->second.end(); ){
				let& p = *listener;
				let isLast = ++listener==kv->second.end();
				p->OnTraces( isLast ? move(traces) : App::Proto::FromServer::Traces{traces} );
			}
		}
		else
			WARNT( ELogTags::QL, "[{}]Could not find trace subscription.", requestId );
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