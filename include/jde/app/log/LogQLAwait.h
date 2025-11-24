#pragma once
#include <jde/fwk/co/Await.h>
#include <jde/ql/types/TableQL.h>
#include <jde/app/log/ArchiveFile.h>

namespace Jde::App{
	struct LogQLAwait final : TAwaitEx<jarray,TAwait<ArchiveFile>::Task>{
		using base = TAwaitEx<jarray,TAwait<ArchiveFile>::Task>;
		LogQLAwait( QL::TableQL&& ql, SRCE )ι:base{sl}, _ql{move(ql)}{}
		α Execute()ι->TAwait<ArchiveFile>::Task;
	private:
		QL::TableQL _ql;
	};
}