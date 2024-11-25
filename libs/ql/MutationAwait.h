#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/types/MutationQL.h>
#include <jde/ql/QLHook.h>

namespace Jde::QL{
	struct MutationAwait final: TAwait<jvalue>{
		MutationAwait( MutationQL mutation, UserPK userPK, SRCE )ι;
	private:
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->TAwait<jvalue>::Task;
		α Stop()ι->MutationAwaits::Task;
		α Start()ι->MutationAwaits::Task;
		const MutationQL _mutation;
		const UserPK _userPK;
	};
}