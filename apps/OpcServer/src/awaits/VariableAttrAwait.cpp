#include "VariableAttrAwait.h"
#include "ServerConfigAwait.h"

namespace Jde::Opc::Server {
	constexpr sv vAttribSql = "select a.vattribute_id, a.specified, a.name, a.description, a.write_mask, a.user_write_mask, a.variant_id, a.data_type, a.value_rank, a.array_dims, a.access_level, a.user_access_level, a.minimum_sampling_interval, a.historizing from {} sn join {} a on sn.oattribute_id=a.oattribute_id where sn.deleted is null and a.deleted is null";
	α VariableAttrAwait::Execute()ι->DB::SelectAwait::Task {
		try {
			let nodeTable = GetViewPtr("server_nodes");
			let vtable = GetViewPtr("variable_attrs");
			auto rows = co_await DS().SelectAsync(DB::Statement{
				{ vtable->Columns, "a" },
				{ {nodeTable->GetColumnPtr("variable_attr_id"), "n", vtable->GetPK(), "a", true} },
				ServerConfigAwait::ServerWhereClause( *nodeTable, "n" )
			}.Move() );

			vector<DB::Value> variants;
			for( auto&& row : rows )
			  variants.emplace_back( row.GetUInt32(6) );
			LoadVariants( move(variants), move(rows) );
		}
		catch (exception& e) {
			ResumeExp(move(e));
		}
	}
	α VariableAttrAwait::LoadVariants(vector<DB::Value>&& pks, vector<DB::Row>&& rows)ι->VariantAwait::Task{
		try{
			flat_map<VariantPK,Variant> variants = pks.size() ? co_await VariantAwait{ move(pks) } : flat_map<VariantPK,Variant>{};
			flat_map<VAttrPK, VariableAttr> attributes; attributes.reserve(rows.size());
			for( auto&& row : rows )
				attributes.try_emplace(row.GetUInt32(0), row, move(variants.at(row.GetUInt32(6))), DataType(row.GetUInt32(7)), Variant::ToArrayDims(move(row.GetString(9))) );
			Resume( move(attributes) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}