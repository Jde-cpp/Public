#include "ReferenceAwait.h"
#include "ServerConfigAwait.h"

#define let const auto

namespace Jde::Opc::Server {
	α ReferenceAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nodeTable = GetViewPtr( "server_node_ids" );
			let table = GetViewPtr( "refs" );
			DB::Statement stmt{
				{ table->Columns, "ref" },
				{ {nodeTable->GetColumnPtr("node_id"), "n", table->GetColumnPtr("source_node_id"), "ref", true} },
				ServerConfigAwait::ServerWhereClause( *nodeTable, "n" )
			};
			auto rows = co_await DS().SelectAsync( stmt.Move() );
			flat_map<NodePK,Reference> references; references.reserve( rows.size() );
			for( auto&& row : rows )
				references.try_emplace( row.GetUInt32(0), move(row) );
			Resume( move(references) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}