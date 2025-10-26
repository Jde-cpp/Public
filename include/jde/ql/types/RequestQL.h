#pragma once
#include "MutationQL.h"
#include "Subscription.h"

namespace Jde::QL{
	struct TableQL; struct Subscription;
	struct RequestQL{
		RequestQL( TableQL&& table )ι:_value{vector<TableQL>{move(table)}}{}
		RequestQL( vector<TableQL>&& tables )ι:_value{move(tables)}{}
		RequestQL( vector<MutationQL>&& mutation )ι:_value{move(mutation)}{}
		RequestQL( vector<Subscription>&& subscriptions )ι:_value{move(subscriptions)}{}
		RequestQL( vector<SubscriptionId>&& unsubscriptions )ι:_value{move(unsubscriptions)}{}

		α IsMutation()Ι->bool{ return _value.index()==1; }
		α Mutations()Ι->const vector<MutationQL>&{ return const_cast<RequestQL*>(this)->Mutations(); }
		α Mutations()ι->vector<MutationQL>&{ ASSERT(_value.index()==1); return get<1>(_value); }
		α IsSubscription()Ι->bool{ return _value.index()==2; }
		α Subscriptions()ι->vector<Subscription>&{ ASSERT(_value.index()==2); return get<2>(_value); }
		α UnSubscribes()ι->vector<SubscriptionId>&{ ASSERT(_value.index()==3); return get<3>(_value); }
		α IsQueries()Ι->bool{ return _value.index()==0; }
		α Queries()Ι->const vector<TableQL>&{ return const_cast<RequestQL*>(this)->Queries(); }
		α Queries()ι->vector<TableQL>&{ ASSERT(_value.index()==0); return get<0>(_value); }
//		α Variables()ι->sp<jobject>&{ return _variables; }
	private:
		variant<vector<TableQL>,vector<MutationQL>, vector<Subscription>,vector<SubscriptionId>> _value;
	};
}