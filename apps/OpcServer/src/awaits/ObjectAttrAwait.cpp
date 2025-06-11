#include "ObjectAttrAwait.h"
#include "ServerConfigAwait.h"

#define let const auto

namespace Jde::Opc::Server {
	α ObjectAttrAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nodeTable = GetViewPtr("server_nodes");
			let table = GetViewPtr( "object_attrs" );
			DB::Statement stmt{
				{ table->Columns, "a" },
				{ {nodeTable->GetColumnPtr("object_attr_id"), "n",table->GetColumnPtr("object_attr_id"), "a", true} },
				ServerConfigAwait::ServerWhereClause( *nodeTable, "n" )
			};
			auto rows = co_await DS().SelectAsync( stmt.Move() );
			flat_map<OAttrPK, ObjectAttr> attrs; attrs.reserve( rows.size() );
			for( auto&& row : rows )
				attrs.try_emplace( row.GetUInt32(0), move(row) );
			Resume( move(attrs) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}