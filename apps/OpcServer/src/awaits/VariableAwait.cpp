#include "VariableAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/DataType.h"
#include "../uaTypes/VariableAttr.h"

#define let const auto
namespace Jde::Opc::Server {
	α VariableAwait::Execute()ι->DB::SelectAwait::Task {
		try {
			let table = GetViewPtr("variable_nodes");
			auto rows = co_await DS().SelectAsync(DB::Statement{
				vector<sp<DB::Column>>{ table->Columns.begin()+1, table->Columns.end() },
				{ table },
				ServerConfigAwait::ServerWhereClause( *table, {} )
			}.Move() );

			vector<DB::Value> variants;
			for( auto&& row : rows ){
				if( !row.IsNull(21) )
			  	variants.emplace_back( row.GetUInt32(21) );
			}
			LoadVariants( move(variants), move(rows) );
		}
		catch(runtime_error& e){
			ResumeExp(move(e));
		}
	}
	α VariableAwait::LoadVariants( vector<DB::Value>&& pks, vector<DB::Row> rows )ι->VariantMembersAwait::Task{
		try{
			auto values = pks.size() ? co_await VariantMembersAwait{ move(pks) } : VariantMembers{};
			auto nodes = ReserveMap<VariablePK, Variable>( rows.size() );
			for( auto&& row : rows ){
				let variantPK = row.GetUInt32Opt(21);
				let dtPK = row.GetUInt32Opt(29);
				let variableDTPK = row.GetUInt32Opt(22);
				let variantDims = row.GetString( 30 );
				let isArray = !variantDims.empty();//stored arrayDimensions: the shape a one-element array cannot state by its count.
				Variable node{
					row,
					GetUAServer().GetTypeDef( row.GetUInt32(12), _sl ),
					variantPK && dtPK ? Variant{ *variantPK, Variant::ToUAValues(DT(*dtPK), Members(values, *variantPK), isArray), Variant::ToArrayDims(variantDims), DT(*dtPK) }.Move() : UA_Variant{},
					DT( variableDTPK.value_or(UA_NS0ID_BASEDATATYPE) ),
					Variant::ToArrayDims( row.GetString(24) )
				};
				node.Browse = GetUAServer().GetBrowse( node.Browse.PK, _sl );
				nodes.try_emplace( nodes.end(), node.PK, move(node) );
			}
			Resume( move(nodes) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}