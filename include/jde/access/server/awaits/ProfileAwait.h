#pragma once
#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/access/access.h>
#include <jde/ql/types/MutationQL.h>

namespace Jde::Access::Server{
	//updateProfile( target:"logs/views", value:"{…}" ) — upsert scoped to the executer; missing/null value deletes the row.
	struct ProfileAwait final : TAwait<jvalue>, noncopyable{
		ProfileAwait( QL::MutationQL m, UserPK executer, SRCE )ι:
			TAwait<jvalue>{ sl },
			_mutation{ move(m) },
			_executer{ executer }
		{}
		α Suspend()ι->void override;
	private:
		α Execute()ι->DB::ExecuteAwait::Task;
		QL::MutationQL _mutation;
		Jde::UserPK _executer;
	};
}
