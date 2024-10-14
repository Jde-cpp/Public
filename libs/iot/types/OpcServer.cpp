#include <jde/iot/types/OpcServer.h>
#include "../../../../Framework/source/db/types/WhereClause.h"

#define var const auto

namespace Jde::Iot{
	OpcServer::OpcServer( const DB::IRow& r )ε:
		Id{ r.GetUInt32(0) },
		Url{ r.GetString(1) },
		CertificateUri{ r.GetString(2) },
		IsDefault{ r.GetBit(3) },
		Name{ r.GetString(4) },
		Target{ r.GetString(5) }
	{}


	α Load( variant<nullptr_t,OpcPK,OpcNK>&& id, HCoroutine h, bool includeDeleted )ι->Task{
		DB::WhereClause where{ includeDeleted ? "" : "deleted is null" };
		vector<DB::object> params;
		var selectAll = id.index()==0;
		if( !selectAll ){
			if( id.index()==1 ){
				where << "id=?";
				params.push_back( get<OpcPK>(id) );
			}
			else if( id.index()==2 ){
				var target = get<OpcNK>( move(id) );
				if( target.size() ){
					where << "target=?";
					params.push_back( move(target) );
				}
				else
					where << "is_default=1";
			}
		}

		try{
			auto y = (co_await DB::SelectCo<vector<OpcServer>>(Jde::format("select id, url, certificate_uri, is_default, name, target from iot_opc_servers {}", where.Move()), params, []( vector<OpcServer>& result, const DB::IRow& r )ε{
				result.push_back(OpcServer{r});
			} )).UP<vector<OpcServer>>();
			if( !selectAll )
				h.promise().SetResult( y->size() ? mu<OpcServer>(move(y->front())) : up<OpcServer>{} );
			else
				h.promise().SetResult( move(y) );
			h.resume();
		}
		catch( IException& e ){
			Resume( move(e), h );
		}
	}

	α OpcServer::Select( variant<nullptr_t,OpcPK,OpcNK> id_, bool includeDeleted )ι->AsyncAwait{
		return AsyncAwait{ [id=move(id_), deleted=includeDeleted](HCoroutine h)mutable->Task {return Load(move(id),h, deleted);} };
	}
}