#pragma once
#include <jde/framework/co/Await.h>
#include <jde/access/usings.h>
#include <jde/web/client/exports.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include <jde/db/Key.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include "../types/ServerCnnctn.h"

namespace Jde::Opc::Gateway{
	//CRD - Insert/Purge/Select from um_providers table
	struct ProviderSelectAwait final : TAwait<Access::ProviderPK>{
		ProviderSelectAwait( string opcId, SRCE )ι:TAwait<Access::ProviderPK>{sl},_opcId{move(opcId)}{};//select
		α Suspend()ι->void override{ Select(); }
	private:
		α Select()ι->TAwait<jobject>::Task;
		string _opcId;
	};
	struct ProviderCreatePurgeAwait : TAwait<Access::ProviderPK>{
		ProviderCreatePurgeAwait( DB::Key opcKey, bool insert, SRCE )ι:TAwait<Access::ProviderPK>{sl},_insert{insert},_opcKey{move(opcKey)}{}
		α Suspend()ι->void override;
	private:
		α Execute( ServerCnnctnPK opcPK )ι->TAwait<vector<ServerCnnctn>>::Task;
		α Insert( str target )ι->TAwait<jobject>::Task;
		α Purge( str target )ι->ProviderSelectAwait::Task;
		α Purge( Access::ProviderPK pk )ι->TAwait<jvalue>::Task;

		bool _insert;
		DB::Key _opcKey;
	};
}