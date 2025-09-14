#include "ObjectAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/ObjectAttr.h"

#define let const auto

namespace Jde::Opc::Server {
	α ObjectAwait::Execute()ι->DB::SelectAwait::Task {
		try {
			let table = GetViewPtr("object_nodes");
			auto rows = co_await DS().SelectAsync(DB::Statement{
				vector<sp<DB::Column>>{ table->Columns.begin()+1, table->Columns.end() },
				{ table },
				ServerConfigAwait::ServerWhereClause( *table, {} )
			}.Move(), _sl );
			flat_map<NodePK, Object> objects;
			for( auto& r : rows ){
				auto typeDef = GetUAServer().GetTypeDef( r.Get<NodePK>(12), _sl );
			  auto& o = objects.try_emplace( objects.end(), r.Get<NodePK>(0), move(r), typeDef )->second;
				o.Browse = GetUAServer().GetBrowse( o.Browse.PK, _sl );
			}
			Resume( move(objects) );
		}
		catch (exception& e) {
			ResumeExp(move(e));
		}
	}
	ObjectInsertAwait::ObjectInsertAwait( Object&& node, SL sl )ι:
		base{ sl },
		_node{ move(node) }
	{}

	α ObjectInsertAwait::Execute()ι->DB::ScalerAwait<NodePK>::Task{
		try{
			auto& ua = GetUAServer();
			BrowseNameAwait::GetOrInsert( _node.Browse );
			if( !_node.IsSystem() )
				_node = ua.AddObject( _node, _sl );
			_node.PK = co_await DS().InsertSeq<NodePK>( DB::InsertClause{
				GetSchema().DBName( "object_insert" ),
				_node.InsertParams()
			} );
			Resume( move(_node) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}