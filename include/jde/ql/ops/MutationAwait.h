#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/QLHook.h>

namespace Jde::QL{
	struct IQL;
	struct MutationAwait final: TAwait<jvalue>{
		MutationAwait( MutationQL mutation, UserPK executer, sp<IQL> ql, SRCE )ι;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		α Stop()ι->MutationAwaits::Task;
		α Start()ι->MutationAwaits::Task;
		MutationQL _mutation;
		const UserPK _executer;
		sp<IQL> _ql;
	};
}