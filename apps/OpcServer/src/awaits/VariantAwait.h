#pragma once
#include "../uaTypes/Node.h"
#include "../uaTypes/Variant.h"

namespace Jde::Opc::Server {
	struct VariantAwait final : TAwaitEx<flat_map<VariantPK,Variant>,DB::SelectAwait::Task> {
		using base=TAwaitEx<flat_map<VariantPK,Variant>,DB::SelectAwait::Task>;
		VariantAwait( vector<DB::Value>&& pks, SRCE )ι: base{ sl }, _pks{ move(pks) }{}
	private:
		α Execute()ι->DB::SelectAwait::Task override;
		vector<DB::Value> _pks;
	};
}