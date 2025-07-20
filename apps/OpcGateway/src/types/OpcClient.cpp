#include "OpcClient.h"
#include <jde/db/IDataSource.h>
#include <jde/db/Row.h>
#include <jde/db/generators/Functions.h>
#include <jde/db/generators/Statement.h>
#include <jde/db/generators/WhereClause.h>
#include "../opcInternal.h"

#define let const auto

namespace Jde::Opc::Gateway{
	OpcClient::OpcClient( DB::Row&& r )ε:
		Id{ r.Get<uint32>(0) },
		Url{ move(r.GetString(1)) },
		CertificateUri{ move(r.GetString(2)) },
		IsDefault{ r.GetBit(3) },
		Name{ move(r.GetString(4)) },
		Target{ move(r.GetString(5)) }
	{}
	OpcClient::OpcClient( jobject&& o )ε:
		Id{ Json::FindNumber<uint32>(o, "id").value_or(0) },
		Url{ Json::FindDefaultSV(o, "url") },
		CertificateUri{ Json::FindDefaultSV(o, "certificate_uri") },
		Description{ Json::FindDefaultSV(o, "description") },
		IsDefault{ Json::FindBool(o, "is_default") },
		Name{ Json::FindDefaultSV(o, "name") },
		Deleted{ Json::FindTimePoint(o, "deleted") },
		Target{ Json::FindDefaultSV(o, "target") }
	{}
	α OpcClient::ToJson()Ι->jobject{
		jobject o;
		o.emplace( "id", Id );
		o.emplace("client_id", Id);
		o.emplace("url", Url);
		o.emplace("certificate_uri", CertificateUri);
		o.emplace("is_default", IsDefault);
		o.emplace("name", Name);
		o.emplace("target", Target);
		o.emplace( "description", Description );
		o.emplace( "deleted", Deleted ? jvalue{ToIsoString(*Deleted)} : jvalue{} );
		return o;
	}

	α OpcClientAwait::Select()ι->DB::SelectAwait::Task{
		let view = GetViewPtr( "clients" );
		DB::WhereClause where;
		if( !_includeDeleted )
			where.Add( view->GetColumnPtr("deleted"), nullptr );
		if( _key ){
			if( _key->IsPrimary() )
				where.Add( view->GetColumnPtr("client_id"), _key->PK() );
			else{
				if( _key->NK().size() )
					where.Add( view->GetColumnPtr("target"), _key->NK() );
				else
					where.Add( view->GetColumnPtr("is_default"), true );
			}
		}
		auto statement = DB::Statement{ {view->GetColumns({"client_id", "url", "certificate_uri", "is_default", "name", "target"})}, {view}, move(where) };
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