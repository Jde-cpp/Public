#pragma once
#include <jde/db/awaits/ScalerAwait.h>
#include <jde/opc/uatypes/BrowseName.h>

namespace Jde::Opc::Server{
	struct BrowseNameAwait final : TAwaitEx<flat_map<BrowseNamePK,BrowseName>,DB::SelectAwait::Task>{
		using base = TAwaitEx<flat_map<BrowseNamePK,BrowseName>,DB::SelectAwait::Task>;
		// if browseName is null, it will return all browse names.  else it will fill in browseName.PK with value, create if necessary.
		BrowseNameAwait( BrowseName* browseName=nullptr, SRCE )ι: base{ sl }, _browseName{ browseName }{}
		α await_ready()ι->bool override;
		α await_resume()ε->flat_map<BrowseNamePK,BrowseName>;
		Ω GetOrInsert( BrowseName& browseName, SRCE )ε->bool;
		Ω GetOrInsert( const jobject&, SRCE )ι->BrowseName;

	private:
		α Execute()ι->DB::SelectAwait::Task override;
		α Create()ι->DB::ScalerAwait<BrowseNamePK>::Task;
		BrowseName* _browseName;
	};
}