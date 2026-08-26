#include "BrowseNameAwait.h"
#include "ServerConfigAwait.h"

#define let const auto

namespace Jde::Opc::Server{
	α BrowseNameAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nameTable = GetViewPtr("browse_names");
			DB::Statement stmt{
				{ GetView("browse_names").Columns },
				nameTable,
				{}
			};
			auto rows = co_await DS().SelectAsync( stmt.Move(), _sl );
			auto names = ReserveMap<BrowseNamePK,BrowseName>( rows.size() );
			for( auto&& row : rows )
				names.try_emplace( row.GetUInt32(0), row.GetUInt32(0), row.GetUInt16(1), row.GetString(2) );
			Resume( move(names) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α BrowseNameAwait::await_resume()ε->flat_map<BrowseNamePK,BrowseName>{
		return Promise() ? base::await_resume() : flat_map<BrowseNamePK,BrowseName>{};
	}
}