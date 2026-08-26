#include "ObjectAwait.h"
#include <jde/db/meta/AppSchema.h>
#include "ServerConfigAwait.h"
#include "../uaTypes/ObjectAttr.h"

#define let const auto
namespace Jde::Opc::Server {
	α ObjectTypeAwait::Execute()ι->DB::SelectAwait::Task {
		try {
			let table = GetViewPtr("object_type_nodes");
			auto rows = co_await DS().SelectAsync( DB::Statement{
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
		catch(runtime_error& e){
			ResumeExp(move(e));
		}
	}
}