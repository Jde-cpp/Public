#include "ConnectionQLAwait.h"
#include <jde/ql/QLAwait.h>
#include "../LocalClient.h"
#include "../WebServer.h"

#define let const auto

namespace Jde::App::Server{
	//{ connections{id programName instanceName hostName created status{memory values}} }
	α ConnectionQLAwait::IsApplicable( QL::TableQL& q )ι->bool{
		return q.JsonName.starts_with( "connection" ) && q.FindTable( "status" );
	}

	α ConnectionQLAwait::Execute()ι->TAwait<flat_map<ConnectionPK, jvalue>>::Task{
		try{
			flat_map<ConnectionPK, jvalue> conStatuses;
			if( auto status = _query.ExtractTable("status"); status ){
				conStatuses = co_await Server::QuerySessions( move(*status), _creds.UserPK(), _sl );
				conStatuses.emplace( AppClient()->ConnectionPK(), IApp::Status() );
			}

			QueryDB( move(conStatuses) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ConnectionQLAwait::QueryDB( flat_map<ConnectionPK, jvalue> conStatuses )ι->TAwait<jvalue>::Task{
		try{
			bool addedConId = _query.AddColumn( "id" );
			_query.ReturnRaw = true;
			auto connections = co_await QL::QLAwait{ move(_query), _creds, QLPtr(), _sl };
			Json::Visit( connections, [addedConId,&conStatuses](jobject& o){
				let connectionPK = QL::AsId<ConnectionPK>( o );
				if( addedConId )
					o.erase( "id" );
				if( auto it=conStatuses.find(connectionPK); it!=conStatuses.end() )
					o["status"] = move( it->second );
			});
			Resume( move(connections) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}