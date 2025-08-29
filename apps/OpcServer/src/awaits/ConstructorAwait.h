#pragma once
#include <jde/db/awaits/ScalerAwait.h>
#include "VariantAwait.h"

#define Result flat_map<NodeId, flat_map<BrowseNamePK, Variant>>
namespace Jde::Opc::Server{
	struct ConstructorAwait final : TAwaitEx<Result,DB::SelectAwait::Task>{
		using base = TAwaitEx<Result,DB::SelectAwait::Task>;
		ConstructorAwait( SRCE )ι: base{ sl }{}
	private:
		α Execute()ι->DB::SelectAwait::Task override;
		α LoadVariants( vector<DB::Row> rows, vector<DB::Value>&& variantPKs )ι->VariantAwait::Task;
	};
}
#undef Result