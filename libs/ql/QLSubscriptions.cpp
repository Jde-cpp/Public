#include <jde/ql/QLSubscriptions.h>
#include <jde/ql/types/MutationQL.h>

#define let const auto
namespace Jde::QL{
	flat_map<string,vector<Subscription>> _subscriptions;
	std::shared_mutex _mutex;
	uint _id{};
	//comes from server.
	//TODO: get tableName from message.  change this to class and derive in client.
	α Subscriptions::Push( const jobject& m, SubscriptionClientId clientId )ι->void{
		Trace{ ELogTags::Test, "Push:  '{}'", serialize(m) };
		sl l{ _mutex };
		for( let& [tableName, tableSubs] : _subscriptions ){
			for( let& sub : tableSubs ){
				for( let listenerClients : sub.Listeners ){
					if( listenerClients.second.contains(clientId) )
						listenerClients.first->OnChange( m, clientId );
				}
			}
		}
	}
	//comes from a mutation query.
	α Subscriptions::Push( const MutationQL& m, jvalue result )ι->void{
		sl l{ _mutex };
		auto tableSubs = _subscriptions.find( m.TableName() );
		if( tableSubs==_subscriptions.end() )
			return;
		jobject available;
		for( auto sub = tableSubs->second.begin(); sub!=tableSubs->second.end(); ++sub ){
			if( m.Type!=sub->Type )
				continue;
			if( available.empty() ){
				if( let array = result.try_as_array(); array && array->size() )
					result = (*array)[0];
				available = result.is_object() ? Json::Combine( m.Args, result.get_object() ) : m.Args;
				Trace{ ELogTags::Test, "result:  '{}', Args: '{}'", serialize(result), serialize(m.Args) }; //TODO!
			}
			jobject j;
			for( auto listenerClients = sub->Listeners.begin(); listenerClients!=sub->Listeners.end(); ++listenerClients ){
				for( let clientId : listenerClients->second ){
					if( j.empty() )
						j[sub->Fields.JsonName] = sub->Fields.TrimColumns( available );
					try{
						Trace{ ELogTags::Test, "OnChange:  '{}', Args: '{}'", serialize(j), serialize(m.Args) };
						listenerClients->first->OnChange( j, clientId );
					}
					catch( std::exception& )
					{}
				}
			}
		}
	}


	α Subscriptions::Add( vector<Subscription>&& subs )ι->jarray{
		jarray y;
		ul _{ _mutex };
		for( auto&& s : subs ){
			s.Id = ++_id;
			y.emplace_back( s.Id );
			_subscriptions.try_emplace( s.TableName ).first->second.emplace_back( move(s) );
		}
		return y;
	}
	α findSubscription( SubscriptionId id )ι->optional<std::pair<flat_map<string,vector<Subscription>>::iterator,vector<Subscription>::iterator>>{
		for( auto pTable = _subscriptions.begin(); pTable!=_subscriptions.end(); ++pTable ){
			if( auto pSubscription = find_if(pTable->second, [&](auto&& s){ return s.Id==id;}); pSubscription!=pTable->second.end() )
				return make_pair( pTable, pSubscription );
		}
		Warning{ ELogTags::QL, "[{}]Could not find subscription.", id };
		return nullopt;
	}
	α Subscriptions::Remove( vector<uint>&& ids )ι->jarray{
		jarray y;
		ul _{ _mutex };
		for( auto&& id : ids ){
			if( auto sub = findSubscription( id ); sub ){
				sub->first->second.erase( sub->second );
				if( sub->first->second.empty() )
					_subscriptions.erase( sub->first );
				y.emplace_back( id );
			}
		}
		return y;
	}
	α Subscriptions::Listen( sp<IListener> listener, SubscriptionId subscriptionId, SubscriptionClientId clientId )ι->void{
		ul _{ _mutex };
		if( auto sub = findSubscription( subscriptionId ); sub )
			sub->second->Listeners.try_emplace( listener ).first->second.emplace( clientId );
	}
}