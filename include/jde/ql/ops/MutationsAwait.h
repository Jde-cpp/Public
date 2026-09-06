#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/IQLSession.h>
#include <jde/ql/types/MutationQL.h>

namespace Jde::QL{
	struct IQL;
	//The mutation half of the engine, beside TablesAwait:  each mutation through a MutationAwait, and what comes back shaped to the
	//client's result request - trimmed to its columns, wrapped in the command name unless the request was raw, one object or a list.
	struct MutationsAwait final: TAwaitEx<jvalue, TAwait<jvalue>::Task>{
		using base = TAwaitEx<jvalue, TAwait<jvalue>::Task>;
		MutationsAwait( vector<MutationQL>&& mutations, QL::Creds creds, sp<IQL>&& ql, SL sl )ι: base{ sl }, _creds{move(creds)}, _ql{move(ql)}, _mutations{move(mutations)}{}
	private:
		α Execute()ι->TAwait<jvalue>::Task override;
		QL::Creds _creds;
		sp<IQL> _ql;
		vector<MutationQL> _mutations;
	};
}