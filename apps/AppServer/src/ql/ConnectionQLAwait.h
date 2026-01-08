#pragma once
#include <jde/ql/types/TableQL.h>

namespace Jde::App::Server{
	struct ConnectionQLAwait final : TAwaitEx<jvalue, TAwait<flat_map<ConnectionPK, jvalue>>::Task>{
		using base = TAwaitEx<jvalue, TAwait<flat_map<ConnectionPK, jvalue>>::Task>;
		ConnectionQLAwait( QL::TableQL&& q, UserPK executer, SRCE )ι: base{sl}, _executer(executer),  _ql(move(q)){}
	private:
		α Execute()ι->TAwait<flat_map<ConnectionPK, jvalue>>::Task override;
		α QueryDB( flat_map<ConnectionPK, jvalue> conStatuses )ι->TAwait<jvalue>::Task;
		UserPK _executer;
		QL::TableQL _ql;
	};
}