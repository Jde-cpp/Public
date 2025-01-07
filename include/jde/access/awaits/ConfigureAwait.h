#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>

namespace Jde::Access{
	struct ConfigureAwait : VoidAwait<>{
		ConfigureAwait( sp<QL::IQL> qlServer, vector<sp<DB::AppSchema>> schemas, UserPK executer )ι:
			Executer{executer},QlServer{qlServer}, Schemas{schemas}{};
		α Suspend()ι->void override;

		UserPK Executer;
		sp<QL::IQL> QlServer;
		vector<sp<DB::AppSchema>> Schemas;
	};
}