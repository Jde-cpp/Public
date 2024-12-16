#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/ql/ql.h>

namespace Jde::Access{
	struct ConfigureAwait : VoidAwait<>{
		ConfigureAwait( sp<QL::IQL> qlServer, vector<string> schemaNames, UserPK executer )ι:
			Executer{executer},QlServer{qlServer}, SchemaNames{schemaNames}{};
		α Suspend()ι->void override;

		UserPK Executer;
		sp<QL::IQL> QlServer;
		vector<string> SchemaNames;
	private:
		//α LoadUsers();
	//vector<AppPK> AppPKs;
	};
}