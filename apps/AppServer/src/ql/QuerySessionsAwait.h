#pragma once
#include <jde/ql/types/TableQL.h>

namespace Jde::App::Server{
	struct ServerSocketSession;
	struct QuerySessionsAwait final: TAwait<flat_map<ConnectionPK, jvalue>>{
		using base = TAwait<flat_map<ConnectionPK, jvalue>>;
		QuerySessionsAwait( QL::TableQL&& q, Jde::UserPK executer, vector<sp<ServerSocketSession>>&& sessions, SRCE )ι:base{sl}, _executer(executer),  _ql(move(q)), _sessions(move(sessions)){}
		α await_ready()ι->bool override{ return _sessions.empty(); }
		α await_resume()ε->flat_map<ConnectionPK, jvalue> override;
	private:
		α Suspend()ι->void override;
		α SessionQuery( sp<ServerSocketSession> session )ι->TAwait<jvalue>::Task;
		Jde::UserPK _executer;
		QL::TableQL _ql;
		flat_map<ConnectionPK, jvalue> _results; mutex _resultsMutex;
		vector<sp<ServerSocketSession>> _sessions;
	};
}