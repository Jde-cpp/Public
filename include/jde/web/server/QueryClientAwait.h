#pragma once
#include <jde/ql/types/TableQL.h>

namespace Jde::Web::Server{
	struct IWebsocketSession;
	struct QueryClientAwait : TAwait<jvalue>{
		QueryClientAwait( QL::TableQL query, UserPK executer, sp<IWebsocketSession> session )ι:_executer(executer), _query(move(query)), _session(move(session)){};
		α Suspend()ι->void override;
	private:
		UserPK _executer;
		QL::TableQL _query;
		sp<IWebsocketSession> _session;
	};
}