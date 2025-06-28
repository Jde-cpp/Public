#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/Row.h>
#include <jde/db/Key.h>
#include <jde/db/awaits/SelectAwait.h>
#include <jde/opc/usings.h>

namespace Jde::Opc{
	struct OpcClient{
		OpcClient( str address )ι:Url{address}{}
		OpcClient( DB::Row&& r )ε;
		OpcClientPK Id;
		string Url;
		string CertificateUri;
		bool IsDefault;
		string Name;
		OpcClientNK Target;
	};

	struct ΓOPC OpcClientAwait final: TAwait<vector<OpcClient>>{
		using base=TAwait<vector<OpcClient>>;
		OpcClientAwait( optional<DB::Key> key=nullopt, bool includeDeleted=false):_key{key}, _includeDeleted{includeDeleted}{};
		α Suspend()ι->void override{ Select(); }
		α Select()ι->DB::SelectAwait::Task;
	private:
		optional<DB::Key> _key;
		bool _includeDeleted;
	};
}