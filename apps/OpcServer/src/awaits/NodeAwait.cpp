#include "NodeAwait.h"
#include "ServerConfigAwait.h"

#define let const auto

namespace Jde::Opc::Server {
	α NodeAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nodeTable = GetViewPtr("server_node_ids");
			DB::Statement stmt{
				{ *nodeTable, {}, {"node_id", "ns", "number", "string", "guid", "bytes"} },
				{ nodeTable },
				ServerConfigAwait::ServerWhereClause(*nodeTable, {})
			};
			auto rows = co_await DS().SelectAsync( stmt.Move() );
			flat_map<NodePK,ExNodeId> nodes; nodes.reserve( rows.size() );
			for( auto&& row : rows ){
				nodes.try_emplace( row.GetUInt(0), row, 1 );
			}
			Resume( move(nodes) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}