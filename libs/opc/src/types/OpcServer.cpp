#include <jde/opc/types/OpcServer.h>
#include <jde/db/IDataSource.h>
#include <jde/db/IRow.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/generators/WhereClause.h>
#include "../opcInternal.h"

#define let const auto

namespace Jde::Opc{
	OpcServer::OpcServer( DB::IRow& r )ε:
		Id{ r.GetUInt32(0) },
		Url{ r.MoveString(1) },
		CertificateUri{ r.MoveString(2) },
		IsDefault{ r.GetBit(3) },
		Name{ r.MoveString(4) },
		Target{ r.MoveString(5) }
	{}

	α OpcServerAwait::Select()ι->DB::RowAwait::Task{
		let view = GetViewPtr( "servers" );
		DB::WhereClause where;
		if( !_includeDeleted )
			where.Add( view->GetColumnPtr("deleted"), nullptr );
		if( _key ){
			if( _key->IsPrimary() )
				where.Add( view->GetColumnPtr("server_id"), _key->PK() );
			else{
				if( _key->NK().size() )
					where.Add( view->GetColumnPtr("target"), _key->NK() );
				else
					where.Add( view->GetColumnPtr("is_default"), true );
			}
		}
		auto statement = DB::Statement{ {view->GetColumns({"server_id", "url", "certificate_uri", "is_default", "name", "target"})}, {view}, move(where) };
		try{
			vector<OpcServer> y;
			let rows = co_await DS()->SelectCo( statement.Move() );
			for( auto& row : rows )
				y.push_back( OpcServer{*row} );
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}