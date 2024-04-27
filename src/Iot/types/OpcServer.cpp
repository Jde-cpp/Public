#include <jde/iot/types/OpcServer.h>
#include "../../../../Framework/source/db/types/WhereClause.h"

namespace Jde::Iot{
	OpcServer::OpcServer( const DB::IRow& r )ι:
		Id{ r.GetUInt32(0) },
		Url{ r.GetString(1) },
		CertificateUri{ r.GetString(2) },
		IsDefault{ r.GetBit(3) },
		Name{ r.GetString(4) },
		Target{ r.GetString(5) }
	{}


	α Load( optional<string> id, HCoroutine h, bool includeDeleted=false )->Task{
		DB::WhereClause where{ includeDeleted ? "" : "deleted is null" };
		vector<DB::object> params;
		if( id )
		{
			if( id->size() )
			{
				where << "target=?";
				params.push_back( *id );
			}
			else
				where << "is_default=1";
		}

		auto y = (co_await DB::SelectCo<vector<OpcServer>>(Jde::format("select id, url, certificate_uri, is_default, name, target from iot_opc_servers {}", where.Move()), params, []( vector<OpcServer>& result, const DB::IRow& r ){
			result.push_back(OpcServer{r});
		} )).UP<vector<OpcServer>>();
		if( id )
			h.promise().SetResult( y->size() ? mu<OpcServer>(move((*y)[0])) : up<OpcServer>{} );
		else
			h.promise().SetResult( move(y) );
		h.resume();
	}
	α OpcServer::Select()ι->AsyncAwait{
		return AsyncAwait{ [](HCoroutine h)->Task {return Load(nullopt,h);} };
	}
	α OpcServer::Select( str id )ι->AsyncAwait{
		return AsyncAwait{ [id](HCoroutine h)->Task {return Load(move(id),h);} };
	}
}