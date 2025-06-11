#include "NodeAwait.h"
#include "ServerConfigAwait.h"

#define let const auto

namespace Jde::Opc::Server {
	α NodeAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nodeTable = GetViewPtr("server_nodes");
			vector<sp<DB::Column>> columns;
			DB::Statement stmt{
				{ *nodeTable, {"node_id", "ns", "number", "string", "guid", "bytes", "namespace_uri", "server_index", "is_global", "parent_node_id", "reference_type_id", "type_definition_id", "object_attr_id", "type_attr_id", "variable_attr_id", "name"} },
				{ nodeTable },
				{{ ServerConfigAwait::ServerWhereClause(*nodeTable, {}),
					DB::WhereClause{nodeTable->FindColumn("deleted"), nullptr}
				}}
			};
			auto rows = co_await DS().SelectAsync( stmt.Move() );
			flat_map<NodePK,Node> nodes; nodes.reserve( rows.size() );
			for( auto&& row : rows )
				nodes.try_emplace( row.GetUInt32(0), move(row) );
			Resume( move(nodes) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}