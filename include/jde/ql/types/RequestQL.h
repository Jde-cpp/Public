#pragma once
#include "MutationQL.h"
#include "Subscription.h"

namespace Jde::QL{
	struct TableQL; struct Subscription;
	//using RequestQL=std::variant<vector<TableQL>,MutationQL>;
	struct RequestQL{
		RequestQL( TableQL&& table )ι:_value{vector<TableQL>{move(table)}}{}
		RequestQL( vector<TableQL>&& tables )ι:_value{move(tables)}{}
		RequestQL( MutationQL&& mutation )ι:_value{move(mutation)}{}
		RequestQL( vector<Subscription>&& subscriptions )ι:_value{move(subscriptions)}{}
		RequestQL( vector<uint>&& unsubscriptions )ι:_value{move(unsubscriptions)}{}

		α IsMutation()Ι->bool{ return _value.index()==1; }
		α Mutation()Ι->const MutationQL&{ return const_cast<RequestQL*>(this)->Mutation(); }
		α Mutation()ι->MutationQL&{ ASSERT(_value.index()==1); return get<1>(_value); }
		α IsSubscription()Ι->bool{ return _value.index()==2; }
		α Subscriptions()ι->vector<Subscription>&{ ASSERT(_value.index()==2); return get<2>(_value); }
		α UnSubscribes()ι->vector<uint>&{ ASSERT(_value.index()==3); return get<3>(_value); }
		α IsTableQL()Ι->bool{ return _value.index()==0; }
		α TableQLs()Ι->const vector<TableQL>&{ return const_cast<RequestQL*>(this)->TableQLs(); }
		α TableQLs()ι->vector<TableQL>&{ ASSERT(_value.index()==0); return get<0>(_value); }
	private:
		std::variant<vector<TableQL>,MutationQL, vector<Subscription>,vector<uint>> _value;
	};
}