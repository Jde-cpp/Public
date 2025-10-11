#include "BrowseNameAwait.h"
#include "ServerConfigAwait.h"
#include <jde/db/generators/InsertClause.h>

#define let const auto

namespace Jde::Opc::Server{
	α BrowseNameAwait::await_ready()ι->bool{
		return _browseName ? GetUAServer().FindBrowse( *_browseName ) : false;
	}
	α BrowseNameAwait::Execute()ι->DB::SelectAwait::Task{
		try{
			let nameTable = GetViewPtr("browse_names");
			DB::Statement stmt{
				{ GetView("browse_names").Columns },
				nameTable,
				{}
			};
			if( _browseName ){
				stmt.Where += {
					{ {nameTable->GetColumnPtr("ns"), _browseName->namespaceIndex},
					  {nameTable->GetColumnPtr("name"), Opc::ToString(_browseName->name)} }
				};
			}
			auto rows = co_await DS().SelectAsync( stmt.Move(), _sl );
			flat_map<BrowseNamePK,BrowseName> names; names.reserve( rows.size() );
			for( auto&& row : rows )
				names.try_emplace( row.GetUInt32(0), row.GetUInt32(0), row.GetUInt16(1), row.GetString(2) );
			if( _browseName ){
				if( names.empty() ){
					Create();
					co_return;
				}
				else
					_browseName->PK = names.begin()->first;
			}
			Resume( move(names) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α BrowseNameAwait::Create()ι->DB::ScalerAwait<BrowseNamePK>::Task{
		_browseName->PK = co_await DS().InsertSeq<BrowseNamePK>( DB::InsertClause{
			GetView("browse_names").InsertProcName(),
			{ {_browseName->namespaceIndex}, {Opc::ToString(_browseName->name)} }
		}, _sl );
		Resume( {} );
	}

	α BrowseNameAwait::GetOrInsert( BrowseName& browseName, SL sl )ε->bool{
		auto& browseNames = GetUAServer()._browseNames;
		for( auto& [pk, name] : browseNames ){
			if( name.namespaceIndex == browseName.namespaceIndex && ToSV(name.name) == ToSV(browseName.name) ){
				browseName.PK = pk;
				return false;
			}
		}
		let browseTable = GetViewPtr("server_browse_names");
		browseName.PK = DS().ScalerSyncOpt<BrowseNamePK>( DB::Statement{  //could be from another server.
			{ browseTable->GetColumnPtr("browse_id") },
			{ browseTable },
			{{ { browseTable->GetColumnPtr("ns"), browseName.namespaceIndex },
				{ browseTable->GetColumnPtr("name"), Opc::ToString(browseName.name) }
			}}
		}.Move() ).value_or( 0 );
		if( browseName.PK )
			return false;

		browseName.PK = DS().InsertSeqSync<BrowseNamePK>(
			DB::InsertClause{ GetView("browse_names").InsertProcName(), {{browseName.namespaceIndex}, {Opc::ToString(browseName.name)}} }, sl
		);
		browseNames.try_emplace( browseName.PK, browseName );
		return true;
	}
	α BrowseNameAwait::GetOrInsert( const jobject& o, SL sl )ι->BrowseName{
		BrowseName browse( o );
		GetOrInsert( browse, sl );
		return browse;
	}
	α BrowseNameAwait::await_resume()ε->flat_map<BrowseNamePK,BrowseName>{
		return Promise() ? base::await_resume() : flat_map<BrowseNamePK,BrowseName>{};
	}
}