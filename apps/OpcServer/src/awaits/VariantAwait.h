#pragma once
#include <jde/db/awaits/ScalerAwait.h>
#include "../uaTypes/Node.h"
#include <jde/opc/uatypes/Variant.h>

namespace Jde::Opc::Server{
	using VariantMembers = flat_map<VariantPK, flat_map<uint,string>>;
	//Moves out one variant's member rows, or an empty set when the row has none - an empty value writes no members, and
	//`values.at(pk)` threw out_of_range there, taking the whole config load down rather than loading that one as null.
	Ξ Members( VariantMembers& values, VariantPK pk )ι->VariantMembers::mapped_type{
		auto p = values.find( pk );
		return p==values.end() ? VariantMembers::mapped_type{} : move( p->second );
	}
	struct VariantMembersAwait final : TAwaitEx<VariantMembers,DB::SelectAwait::Task>{
		using base=TAwaitEx<VariantMembers,DB::SelectAwait::Task>;
		VariantMembersAwait( vector<DB::Value> pks, SRCE )ι: base{ sl }, _pks{ move(pks) }{}
	private:
		α Execute()ι->DB::SelectAwait::Task override;
		vector<DB::Value> _pks;
	};

	struct VariantAwait final : TAwaitEx<flat_map<VariantPK,Variant>,VariantMembersAwait::Task>, noncopyable{
		using base=TAwaitEx<flat_map<VariantPK,Variant>,VariantMembersAwait::Task>;
		VariantAwait( vector<DB::Value>&& pks, SRCE )ι: base{ sl }, _pks{ move(pks) }{}
	private:
		α Execute()ι->VariantMembersAwait::Task override;
		α SelectVariants( VariantMembers values )->DB::SelectAwait::Task;
		vector<DB::Value> _pks;
	};

	#define ExecuteResult DB::ScalerAwait<VariantPK>::Task
	struct VariantInsertAwait final : TAwaitEx<VariantPK,ExecuteResult>{
		using base=TAwaitEx<VariantPK,ExecuteResult>;
		VariantInsertAwait( Variant v, SRCE )ι: base{ sl }, _variant{ move(v) }{}
	private:
		α Execute()ι->ExecuteResult override;
		Variant _variant;
	};
	#undef ExecuteResult
}