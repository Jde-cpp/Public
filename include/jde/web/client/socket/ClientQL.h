#pragma once
//#include <jde/app/client/AppClientSocketSession.h>
#include <jde/ql/IQL.h>

namespace Jde::Web::Client{
	struct IClientSocketSession;
	struct ClientQL final : std::enable_shared_from_this<ClientQL>, QL::IQL{
		ClientQL( sp<IClientSocketSession> session )ι:_session{session}{}
		α Query( string query, UserPK executer, SRCE )ι->up<TAwait<jvalue>> override;
		α QueryObject( string query, UserPK executer, SRCE )ε->up<TAwait<jobject>> override;
		α QueryArray( string query, UserPK executer, SRCE )ε->up<TAwait<jarray>> override;
		α Subscribe( string&& query, QL::SubscriptionClientId clientId, UserPK executer, SRCE )ε->up<TAwait<vector<QL::SubscriptionId>>> override;
	private:
		sp<IClientSocketSession> _session;
	};
}
