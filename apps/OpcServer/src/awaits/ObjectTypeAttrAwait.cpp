#include "ObjectTypeAttrAwait.h"
#include "ServerConfigAwait.h"

#define let const auto

namespace Jde::Opc::Server {
	α ObjectTypeAttrAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nodeTable = GetViewPtr("server_nodes");
			let table = GetViewPtr( "object_type_attrs" );
			DB::Statement stmt{
				{ table->Columns, "a" },
				{ {nodeTable->GetColumnPtr("object_type_attr_id"), "n", table->GetColumnPtr("object_type_attr_id"), "a", true} },
				ServerConfigAwait::ServerWhereClause( *nodeTable, "n" )
			};
			auto rows = co_await DS().SelectAsync( stmt.Move() );
			flat_map<OTypeAttrPK,ObjectTypeAttr> attributes; attributes.reserve( rows.size() );
			for( auto&& row : rows )
				attributes.try_emplace( row.GetUInt32(0), move(row) );
			Resume( move(attributes) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}