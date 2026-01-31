#pragma once
#include "UnsubscribeAwait.h"

namespace Jde::Access{ struct Authorize; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct TableQL; struct MutationQL;
	struct IQL : std::enable_shared_from_this<IQL>{
		β Authorizer()ε->Access::Authorize& = 0;
		β AuthorizerPtr()ε->sp<Access::Authorize> = 0;
		β CustomQuery( QL::TableQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> = 0;
		β CustomMutation( QL::MutationQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> = 0;
		[[nodiscard]] β Query( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jvalue>> =0;
		[[nodiscard]] β QueryObject( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> =0;
		[[nodiscard]] β QueryArray( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> =0;
		β Upsert( string query, jobject variables, UserPK executer )ε->jarray=0;
		β Schemas()Ι->const vector<sp<DB::AppSchema>>& = 0;

		[[nodiscard]] α Unsubscribe( flat_set<SubscriptionId>&& ids, SRCE )ε->UnsubscribeAwait{ return UnsubscribeAwait{ move(ids), shared_from_this(), sl }; }
		[[nodiscard]] β Subscribe( string&& query, jobject variables, sp<IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> = 0;
	};
}