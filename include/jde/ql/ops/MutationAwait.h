#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/IQLSession.h>
#include <jde/ql/types/MutationQL.h>

namespace Jde::QL{
	struct IQL;
	struct MutationAwait final: TAwait<jvalue>{
		MutationAwait( MutationQL mutation, Creds creds, sp<IQL> ql, SRCE )ι;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		MutationQL _mutation;
		Creds _creds;
		sp<IQL> _ql;
	};
}