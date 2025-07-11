#pragma once
#include <jde/ql/IQL.h>

namespace Jde::Web::Client{
	struct IClientSocketSession;
	struct ClientQL final : QL::IQL{
		ClientQL( sp<IClientSocketSession> session )ι:_session{session}{}
		α Query( string query, UserPK executer, bool returnRaw=true, SRCE )ι->up<TAwait<jvalue>> override;
		α QueryObject( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jobject>> override;
		α QueryArray( string query, UserPK executer, bool returnRaw=true, SRCE )ε->up<TAwait<jarray>> override;
		α MutateIfNotFound( string /*query*/, UserPK /*executer*/ )ε->jarray{ throw Exception("Not implemented."); }
		α Subscribe( string&& query, sp<QL::IListener> listener, UserPK executer, SRCE )ε->up<TAwait<vector<QL::SubscriptionId>>> override;
		β Upsert( string /*query*/, UserPK /*executer*/ )ε->jarray{ throw Exception{"Not implemented."}; }
	private:
		sp<IClientSocketSession> _session;
	};
}