#pragma once
#include <jde/ql/IQL.h>

namespace Jde::Web::Client{
	struct IClientSocketSession;
	struct ClientQL final : QL::IQL, noncopyable{
		ClientQL( sp<IClientSocketSession> session, sp<Access::Authorize> authorize )ι:_session{session}, _authorize{authorize}{}
		α Authorizer()ε->Access::Authorize&{ return *_authorize; }
		α AuthorizerPtr()ε->sp<Access::Authorize>{ return _authorize; }
		α CustomQuery( QL::TableQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override{ return nullptr; }
		α CustomMutation( QL::MutationQL& ql, UserPK executer, SL sl )ι->up<TAwait<jvalue>> override{ return nullptr; }
		α Query( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ι->up<TAwait<jvalue>> override;
		α QueryObject( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> override;
		α QueryArray( string query, jobject variables, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> override;
		α MutateIfNotFound( string /*query*/, UserPK /*executer*/ )ε->jarray{ throw Exception("Not implemented."); }
		α Schemas()Ι->const vector<sp<DB::AppSchema>>&{ return _schemas;}
		α Subscribe( string&& query, jobject variables, sp<QL::IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<QL::SubscriptionId>>> override;
		β Upsert( string /*query*/, jobject /*variables*/, UserPK /*executer*/ )ε->jarray{ throw Exception{"Not implemented."}; }
	private:
		const vector<sp<DB::AppSchema>> _schemas; //empty
		sp<IClientSocketSession> _session;
		sp<Access::Authorize> _authorize;
	};
}