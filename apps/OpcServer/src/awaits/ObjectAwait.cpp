#include "ObjectAwait.h"
#include <jde/db/meta/AppSchema.h>
#include <stdexcept>
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
		catch(runtime_error& e){
			ResumeExp(move(e));
		}
	}
}