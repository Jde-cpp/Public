#pragma once
#include <jde/db/awaits/ScalerAwait.h>
#include <jde/opc/uatypes/BrowseName.h>

namespace Jde::Opc::Server{
	//Loads every browse name for the DB-backed address space (ServerConfigAwait).
	struct BrowseNameAwait final : TAwaitEx<flat_map<BrowseNamePK,BrowseName>,DB::SelectAwait::Task>{
		using base = TAwaitEx<flat_map<BrowseNamePK,BrowseName>,DB::SelectAwait::Task>;
		BrowseNameAwait( SRCE )ι: base{ sl }{}
		α await_resume()ε->flat_map<BrowseNamePK,BrowseName> override;
	private:
		α Execute()ι->DB::SelectAwait::Task override;
	};
}