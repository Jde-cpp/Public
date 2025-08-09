#pragma once
#ifndef OPC_CLIENT
#define OPC_CLIENT
#include <jde/framework/coroutine/Await.h>
#include <jde/db/Row.h>
#include <jde/db/Key.h>
#include <jde/db/awaits/SelectAwait.h>
#include <jde/opc/usings.h>

namespace Jde::Opc::Gateway{
	struct ServerCnnctn final{
		ServerCnnctn( str address )ι:Url{address}{}
		ServerCnnctn( DB::Row&& r )ε;
		ServerCnnctn( jobject&& o )ε;
		α ToJson()Ι->jobject;
		ServerCnnctnPK Id;
		string Url;
		string CertificateUri;
		string Description;
		bool IsDefault;
		string Name;
		optional<TimePoint> Deleted;
		ServerCnnctnNK Target;
	};

	struct ServerCnnctnAwait final: TAwait<vector<ServerCnnctn>>{
		using base=TAwait<vector<ServerCnnctn>>;
		ServerCnnctnAwait( optional<DB::Key> key=nullopt, bool includeDeleted=false):_key{key}, _includeDeleted{includeDeleted}{};
		α Suspend()ι->void override{ Select(); }
		α Select()ι->DB::SelectAwait::Task;
	private:
		optional<DB::Key> _key;
		bool _includeDeleted;
	};
}
#endif