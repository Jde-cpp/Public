#include <jde/opc/types/OpcClient.h>
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/generators/WhereClause.h>
#include "../opcInternal.h"

#define let const auto

namespace Jde::Opc{
	OpcClient::OpcClient( DB::Row&& r )ε:
		Id{ r.Get<uint32>(0) },
		Url{ move(r.GetString(1)) },
		CertificateUri{ move(r.GetString(2)) },
		IsDefault{ r.GetBit(3) },
		Name{ move(r.GetString(4)) },
		Target{ move(r.GetString(5)) }
	{}

	α OpcClientAwait::Select()ι->DB::SelectAwait::Task{
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
			vector<OpcClient> y;
			auto rows = co_await DS()->SelectAsync( statement.Move() );
			for( auto&& row : rows )
				y.push_back( OpcClient{move(row)} );
			Resume( move(y) );
		}
		catch( IException& e ){
			ResumeExp( move(e) );
		}
	}
}