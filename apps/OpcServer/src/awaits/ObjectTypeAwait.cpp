#include "ObjectAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/ObjectAttr.h"

namespace Jde::Opc::Server {
	α ObjectTypeAwait::Execute()ι->DB::SelectAwait::Task {
		try {
			let table = GetViewPtr("object_type_nodes");
			auto rows = co_await DS().SelectAsync(DB::Statement{
				vector<sp<DB::Column>>{ table->Columns.begin()+1, table->Columns.end() },
				{ table },
				ServerConfigAwait::ServerWhereClause( *table, {} )
			}.Move() );
			flat_map<NodePK, sp<ObjectType>> objectTypes;
			for( auto&& row : rows ){
				auto type = objectTypes.try_emplace( objectTypes.end(), row.GetUInt(0), ms<ObjectType>(row) )->second;
				type->Browse = GetUAServer().GetBrowse( type->Browse.PK );
			}
			Resume( move(objectTypes) );
		}
		catch (exception& e) {
			ResumeExp(move(e));
		}
	}
	ObjectTypeInsertAwait::ObjectTypeInsertAwait( sp<ObjectType> node, SL sl )ι:
		base{ sl },
		_node{ move(node) }
	{}

	α ObjectTypeInsertAwait::Execute()ι->DB::ScalerAwait<NodePK>::Task{
		try{
			auto& ua = GetUAServer();
			BrowseNameAwait::GetOrInsert( _node->Browse );
			if( !_node->IsSystem() )
				ua.AddObjectType( _node, _sl );
			_node->PK = co_await DS().InsertSeq<NodePK>( DB::InsertClause{
				GetSchema().DBName( "object_type_insert" ),
				_node->InsertParams()
			} );
			Resume( move(_node) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}