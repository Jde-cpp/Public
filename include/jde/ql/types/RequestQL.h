#pragma once
#include "MutationQL.h"
#include "Subscription.h"

namespace Jde::QL{
	struct TableQL; struct Subscription;
	struct RequestQL{
		RequestQL( TableQL&& table )ι:_value{ vector<TableQL>{move(table)} }{}
		RequestQL( vector<TableQL>&& tables )ι:_value{ move(tables) }{}
		RequestQL( vector<MutationQL>&& mutation )ι:_value{ move(mutation) }{}
		RequestQL( vector<Subscription>&& subscriptions )ι:_value{ move(subscriptions) }{}
		RequestQL( vector<SubscriptionId>&& unsubscriptions )ι:_value{ move(unsubscriptions) }{}

		α IsMutation()Ι->bool{ return _value.index()==1; }
		α Mutations()Ι->const vector<MutationQL>&{ return const_cast<RequestQL*>(this)->Mutations(); }
		α Mutations()ι->vector<MutationQL>&{ ASSERT(_value.index()==1); return get<1>(_value); }
		α IsSubscription()Ι->bool{ return _value.index()==2; }
		α Subscriptions()Ι->const vector<Subscription>&{ ASSERT(_value.index()==2); return get<2>(_value); }
		α UnSubscribes()Ι->const vector<SubscriptionId>&{ ASSERT(_value.index()==3); return get<3>(_value); }
		α IsQueries()Ι->bool{ return _value.index()==0; }
		α Queries()Ι->const vector<TableQL>&{ return const_cast<RequestQL*>(this)->Queries(); }
		α Queries()ι->vector<TableQL>&{ ASSERT(_value.index()==0); return get<0>(_value); }
		α ToString()Ι->string;
	private:
		variant<vector<TableQL>,vector<MutationQL>, vector<Subscription>,vector<SubscriptionId>> _value;
	};

	Ξ RequestQL::ToString()Ι->string{
		string y;
		if( IsQueries() ){
			for( const auto& q : Queries() )
				y += q.ToString();
		}
		else if( IsMutation() ){
			for( const auto& m : Mutations() )
				y += m.ToString();
		}
		else if( IsSubscription() ){
			for( const auto& s : Subscriptions() )
				y += s.TableName + " " + std::to_string( underlying(s.Type) ) + " { " + s.Fields.ToString() + " } ";
		}
		else if( UnSubscribes().size() )
			y = "{ unsubscribe {" + Str::Join(UnSubscribes(), ", ") + "} }";
		return y;
	}
}