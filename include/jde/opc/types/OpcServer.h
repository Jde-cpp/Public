#pragma once
#include <jde/framework/coroutine/Await.h>
#include <jde/db/IRow.h>
#include <jde/db/Key.h>
#include <jde/db/awaits/RowAwait.h>

namespace Jde::Opc{
	struct OpcServer{
		OpcServer( str address )ι:Url{address}{}
		OpcServer( DB::IRow& r )ε;
		OpcPK Id;
		string Url;
		string CertificateUri;
		bool IsDefault;
		string Name;
		OpcNK Target;
	};

	struct ΓOPC OpcServerAwait final: TAwait<vector<OpcServer>>{
		using base=TAwait<vector<OpcServer>>;
		OpcServerAwait( optional<DB::Key> key=nullopt, bool includeDeleted=false):_key{key}, _includeDeleted{includeDeleted}{};
		α Suspend()ι->void override{ Select(); }
		α Select()ι->DB::RowAwait::Task;
	private:
		optional<DB::Key> _key;
		bool _includeDeleted;
	};
}