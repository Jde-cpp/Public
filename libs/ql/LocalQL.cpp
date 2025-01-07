#include "LocalQL.h"
#include <jde/ql/ql.h>
#include <jde/ql/QLSubscriptions.h>
#include <jde/ql/SubscriptionAwait.h>

#define let const auto

namespace Jde::QL{
	constexpr ELogTags _tags{ ELogTags::QL };

	struct SubscribeQueryAwait : TAwaitEx<vector<SubscriptionId>,TAwait<jobject>::Task>{
		using Await = TAwait<jobject>;
		using base = TAwaitEx<vector<QL::SubscriptionId>,typename Await::Task>;
		SubscribeQueryAwait( up<Await> await )ι: base{await->Source()}, _await{move(await)}{}
		α Execute()ι->Await::Task override{
			try{
				let subs = co_await *_await;
				auto y = Json::FromArray<QL::SubscriptionId>( Json::AsArray( subs, "subscriptions") );
				base::Resume( move(y) );
			}
			catch( IException& e ){
				base::ResumeExp( move(e) );
			}
		}
		up<Await> _await;
	};
	α LocalQL::Subscribe( string&& query, SubscriptionClientId _, UserPK executer, SL sl )ε->up<TAwait<vector<SubscriptionId>>>{
		return mu<SubscribeQueryAwait>( QueryObject(move(query), executer, sl) );
	}
}