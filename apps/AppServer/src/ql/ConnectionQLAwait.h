#pragma once
#include <jde/ql/types/TableQL.h>
#include <jde/ql/IQLSession.h>

namespace Jde::App::Server{
	struct ConnectionQLAwait final : TAwaitEx<jvalue, TAwait<flat_map<ConnectionPK, jvalue>>::Task>{
		using base = TAwaitEx<jvalue, TAwait<flat_map<ConnectionPK, jvalue>>::Task>;
		ConnectionQLAwait( QL::TableQL&& q, QL::Creds&& creds, SRCE )ι: base{sl}, _creds(creds),  _ql(move(q)){}
	private:
		α Execute()ι->TAwait<flat_map<ConnectionPK, jvalue>>::Task override;
		α QueryDB( flat_map<ConnectionPK, jvalue> conStatuses )ι->TAwait<jvalue>::Task;
		QL::Creds _creds;
		QL::TableQL _ql;
	};
}