#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/access/usings.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include <jde/db/Key.h>
#include <jde/db/awaits/RowAwait.h>
#include <jde/web/client/socket/ClientSocketAwait.h>
#include <jde/opc/async/ConnectAwait.h>

namespace Jde::Opc{
	struct ΓOPC AuthenticateAwait : TAwait<Web::FromServer::SessionInfo>{
		using base = TAwait<Web::FromServer::SessionInfo>;
		AuthenticateAwait( str loginName, str password, str opcNK, str endpoint, bool isSocket, SRCE )ι;
		α Suspend()ι->void override{ Execute(); }
		α Execute()ι->ConnectAwait::Task;
	private:
		α CheckProvider()ι->TAwait<Access::ProviderPK>::Task;
		α AddSession( Access::ProviderPK providerPK )ι->Web::Client::ClientSocketAwait<Web::FromServer::SessionInfo>::Task;
		string _loginName; string _password; string _opcNK; string _endpoint; bool _isSocket;
	};
	//CRD - Insert/Purge/Select from um_providers table
	struct ΓOPC ProviderSelectAwait final : TAwait<Access::ProviderPK>{
		ProviderSelectAwait( string opcId, SRCE )ι:TAwait<Access::ProviderPK>{sl},_opcId{move(opcId)}{};//select
		α Suspend()ι->void override{ Select(); }
	private:
		α Select()ι->TAwait<jobject>::Task;
		string _opcId;
	};
	struct ΓOPC ProviderCreatePurgeAwait : TAwait<Access::ProviderPK>{
		ProviderCreatePurgeAwait( DB::Key opcKey, bool insert, SRCE )ι:TAwait<Access::ProviderPK>{_sl},_insert{insert},_opcKey{move(opcKey)}{}
		α Suspend()ι->void override;
	private:
		α Execute( OpcPK opcPK )ι->OpcServerAwait::Task;
		α Insert( str target )ι->TAwait<jobject>::Task;
		α Purge( str target )ι->ProviderSelectAwait::Task;
		α Purge( Access::ProviderPK pk )ι->TAwait<jvalue>::Task;

		bool _insert;
		DB::Key _opcKey;
	};

	ΓOPC α Credentials( SessionPK sessionId, str opcId )ι->tuple<string,string>;
	ΓOPC α Logout( SessionPK sessionId )ι->void;
}