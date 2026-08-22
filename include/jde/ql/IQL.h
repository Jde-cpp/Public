#pragma once
#include <jde/fwk/co/Await.h>
#include "LocalSubscriptions.h"
#include "IQLSession.h"

namespace Jde::Access{ struct Authorize; }
namespace Jde::DB{ struct AppSchema; }
namespace Jde::QL{
	struct TableQL; struct MutationQL;

	struct IQL : std::enable_shared_from_this<IQL>{
		β Authorizer()ε->Access::Authorize& = 0;
		β AuthorizerPtr()ε->sp<Access::Authorize> = 0;
		β CustomQuery( QL::TableQL& ql, Creds executer, SL sl )ι->up<TAwait<jvalue>> = 0;
		β CustomMutation( QL::MutationQL& ql, Creds executer, SL sl )ι->up<TAwait<jvalue>> = 0;
		β LogQuery( QL::TableQL&& ql, Creds executer, SRCE )ε->up<TAwait<jvalue>> = 0;
		β LogSettingsQuery( QL::TableQL&& ql, Creds executer, SRCE )ε->up<TAwait<jvalue>> = 0;
		β StatusQuery( QL::TableQL&& ql, Creds executer, SRCE )ε->jobject = 0;
		[[nodiscard]] β Query( string query, jobject vars, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jvalue>> =0;
		[[nodiscard]] β QueryObject( string query, jobject vars, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> =0;
		[[nodiscard]] β QueryArray( string query, jobject vars, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> =0;
		β Upsert( string query, jobject vars, UserPK executer )ε->jarray=0;
		β Schemas()Ι->const vector<sp<DB::AppSchema>>& = 0;

		β Unsubscribe( sp<IListener> listener, flat_set<SubscriptionId> ids, SL=SRCE_CUR )ι->void{
			Subscriptions::StopListen( listener, vector<SubscriptionId>{ids.begin(), ids.end()} );
		}
		[[nodiscard]] β Subscribe( string&& query, jobject vars, sp<IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<SubscriptionId>>> = 0;
	};
}