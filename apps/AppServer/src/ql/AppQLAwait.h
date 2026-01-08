#pragma once
#include <jde/ql/types/RequestQL.h>

namespace Jde::App::Server{
	struct AppQLAwait final : TAwaitEx<jvalue, TAwait<jvalue>::Task>{
		using base = TAwaitEx<jvalue, TAwait<jvalue>::Task>;
		AppQLAwait( QL::RequestQL&& ql, UserPK executer, bool raw, SRCE )ι:base{sl}, _executer(executer), _ql(move(ql)), _raw(raw){}
	private:
		α Execute()ι->TAwait<jvalue>::Task override;
		UserPK _executer;
		QL::RequestQL _ql;
		bool _raw;
	};
}