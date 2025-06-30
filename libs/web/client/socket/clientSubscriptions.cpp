#include <jde/web/client/socket/clientSubscriptions.h>
#include <jde/ql/types/Subscription.h>

#define let const auto
namespace Jde::Web::Client{
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
	α Subscriptions::ListenRemote( sp<QL::IListener> listener, vector<QL::Subscription>&& subs )ι->void{
		ul _{ _mutex };
		for( auto&& s : subs )
			_subs.try_emplace( s.Id ).first->second.emplace( listener );
	}
	α Subscriptions::OnWebsocketReceive( const jobject& m, QL::SubscriptionId clientId )ι->void{
		sl l{ _mutex };
		if( auto kv = _subs.find( clientId ); kv!=_subs.end() ){
			for( let& listener : kv->second )
				listener->OnChange( m, clientId );
		}
		else
			Warning{ ELogTags::QL, "[{}]Could not find subscription.", clientId };
	}
}