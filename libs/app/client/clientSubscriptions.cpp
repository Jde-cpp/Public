#include <jde/app/client/clientSubscriptions.h>
#include <jde/ql/types/Subscription.h>
#include <jde/app/client/IAppClient.h>

#define let const auto
namespace Jde::App::Client{
	static constexpr ELogTags _subTags{ ELogTags::QL | ELogTags::Client };
	flat_map<QL::SubscriptionId,flat_set<sp<QL::IListener>>> _subs; std::shared_mutex _mutex;
	//Separate from _subs and deliberately not cleared with it: _subs is what this socket is carrying, _requests is what
	//the process asked for.  Only the first dies with the socket.
	vector<Subscriptions::Request> _requests; std::mutex _requestMutex;
	α Subscriptions::Clear()ι->void{
		ul _{ _mutex };
		_subs.clear();
	}
	α Subscriptions::Remember( string query, jobject variables, sp<QL::IListener> listener, const flat_set<QL::SubscriptionId>& ids )ι->void{
		lg _{ _requestMutex };
		//Matched on what was asked for, not on the ids it produced: a replay re-issues the same query and gets new ids, and
		//must update the existing entry rather than add a second one that would then be replayed twice.
		auto p = find_if( _requests, [&](let& r){ return r.Listener==listener && r.Query==query && r.Variables==variables; } );
		if( p==_requests.end() )
			_requests.push_back( Request{move(query), move(variables), move(listener), ids} );
		else
			p->Ids = ids;
	}
	α Subscriptions::Forget( const sp<QL::IListener>& listener, const flat_set<QL::SubscriptionId>& ids )ι->void{
		lg _{ _requestMutex };
		std::erase_if( _requests, [&](auto& r){
			if( r.Listener!=listener )
				return false;
			if( ids.empty() )
				return true;
			std::erase_if( r.Ids, [&](let id){ return ids.contains(id); } );
			return r.Ids.empty();//a request only goes when nothing it produced is still subscribed.
		});
	}
	α Subscriptions::Remembered()ι->vector<Request>{
		lg _{ _requestMutex };
		return _requests;
	}
	//Subscribe only builds the await - the write happens when it is suspended on - so each one has to be awaited, and that
	//has to happen off the connect path.  Sequential rather than fanned out: they share one socket, and an ack has to land
	//before the next request reuses the id space.
	Ω replay( sp<IAppClient> client, vector<Subscriptions::Request> requests )ι->Web::Client::ClientSocketAwait<jarray>::Task{
		uint done{};
		for( let& r : requests ){
			try{
				co_await client->Subscribe( string{r.Query}, r.Variables, r.Listener );
				++done;
			}
			catch( runtime_error& e ){
				WARNT( _subTags, "Could not replay subscription '{}': {}", r.Query, e.what() );
			}
		}
		INFOT( _subTags, "Replayed {} of {} subscription request(s) on the new session.", done, requests.size() );
	}
	α Subscriptions::Replay( sp<IAppClient> client )ι->uint{
		auto requests = Remembered();
		let count = (uint)requests.size();//answered before any await: Client::Connect needs to know now whether this is a reconnect.
		if( count )
			replay( move(client), move(requests) );
		return count;
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
	//The listeners for an id, copied out from under the lock.  Callbacks must not run while _mutex is held: a listener that
	//subscribes or unsubscribes in response to an event re-enters ListenRemote/StopListenRemote/Clear, all of which take
	//the same mutex exclusively - and a shared_mutex is not recursive, so that is a hang (formally, UB).  Copying the
	//shared_ptrs also keeps every listener alive across its own callback, which holding the lock only did by accident.
	//The trade is that a listener can be dropped between the snapshot and the call, so it may see one event after
	//unsubscribing - which is the ordinary cost of not holding a lock across a callback, and cheaper than the alternative.
	Ω listenersFor( QL::SubscriptionId id )ι->flat_set<sp<QL::IListener>>{
		sl l{ _mutex };
		let kv = _subs.find( id );
		return kv==_subs.end() ? flat_set<sp<QL::IListener>>{} : kv->second;//_subs never holds an empty set - StopListenRemote erases the key when the last listener goes - so empty means absent.
	}
	α Subscriptions::OnTraces( App::Proto::FromServer::Traces&& traces, QL::SubscriptionId requestId )ι->void{
		auto listeners = listenersFor( requestId );
		if( listeners.empty() ){
			WARNT( ELogTags::QL, "[{}]Could not find trace subscription.", requestId );
			return;
		}
		for( auto listener = listeners.begin(); listener!=listeners.end(); ){
			let& p = *listener;
			let isLast = ++listener==listeners.end();
			p->OnTraces( isLast ? move(traces) : App::Proto::FromServer::Traces{traces} );//the last one gets the original; the rest a copy.
		}
	}
	α Subscriptions::OnWebsocketReceive( const jobject& m, QL::SubscriptionId clientId )ι->void{
		let listeners = listenersFor( clientId );
		if( listeners.empty() ){
			WARNT( ELogTags::QL, "[{}]Could not find subscription.", clientId );
			return;
		}
		for( let& listener : listeners ){
			try{
				listener->OnChange( m, clientId );
			}
			catch( runtime_error& )
			{}
		}
	}
}