#include "ConstructorAwait.h"
#include "ServerConfigAwait.h"

#define let const auto
namespace Jde::Opc::Server{
	α ConstructorAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let table = GetViewPtr( "constructors" );
			let nodeIdTable = GetViewPtr("server_node_ids");
			DB::SelectClause cols{ *nodeIdTable, "n", {"ns", "number", "string", "guid", "bytes"} };
			cols += DB::SelectClause{ *table, "c", {"browse_id", "variant_id"} };
			auto rows = co_await DS().SelectAsync( DB::Statement{
				cols,
				{ DB::Join{nodeIdTable->GetColumnPtr("node_id"), "n", table->GetColumnPtr("node_id"), "c", true} },
				ServerConfigAwait::ServerWhereClause( *nodeIdTable, "n" )
			}.Move(), _sl );
			if( rows.empty() ){
				Resume( {} );
				co_return;
			}
			vector<DB::Value> variantPKs;
			for( let& row : rows )
				variantPKs.push_back( row[6] );
			LoadVariants( move(rows), move(variantPKs) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ConstructorAwait::LoadVariants( vector<DB::Row> rows, vector<DB::Value>&& variantPKs )ι->VariantAwait::Task{
		try{
			auto variants = co_await VariantAwait{ move(variantPKs), _sl };
			flat_map<NodeId, flat_map<BrowseNamePK, Variant>> y;
			for( auto&& row : rows ){
				y.try_emplace( y.end(), NodeId{row, 0} )->second.try_emplace(
					row.GetUInt32(5),
					move(variants.at(row.GetUInt32(6))) );
			}
			Resume( move(y) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}