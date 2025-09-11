#include "NodeQLAwait.h"
#include <jde/framework/collections/collections.h>
#include <jde/opc/uatypes/Variant.h>
#include "../UAClient.h"
#include "../async/ConnectAwait.h"
#include "../uatypes/uaTypes.h"


#define let const auto
namespace Jde::Opc::Gateway{

	α NodeQLAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		auto opcId = Json::FindString(_query.Args,"opc").value_or("");
		try{
			Trace( ELogTags::Test, "opc: {}, UserPK: {:x}, SessionId: {:x}", opcId, _executer.Value, _sessionPK );
			_client = co_await ConnectAwait{ move(opcId), _sessionPK, _executer, _sl };
			optional<NodeId> nodeId;
			jobject jnode;
			if( auto p = Json::FindSV(_query.Args,"path"); p ){
				auto ex = _client->ToNodeId( *p );
				nodeId = ex.nodeId;
				if( _query.FindColumn("eid") )
					jnode = ex.ToJson();
				else if( _query.FindColumn("id") )
					jnode = nodeId->ToJson();
			}
			else
				nodeId = NodeId::FromJson( _query.Args );
			AddAttributes( move(*nodeId), move(jnode) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	flat_map<string,UA_AttributeId> _attributes{
		{"description", UA_ATTRIBUTEID_DESCRIPTION},
		{"value", UA_ATTRIBUTEID_VALUE},
		{"browseName", UA_ATTRIBUTEID_BROWSENAME},
		{"name", UA_ATTRIBUTEID_DISPLAYNAME},
		{"dataType", UA_ATTRIBUTEID_DATATYPE}
	};
	α NodeQLAwait::AddAttributes( NodeId&& nodeId, jobject&& jnode )ι->void{
		try{
			let size = _query.Columns.size();
			auto ids = Reserve<UA_AttributeId>( size );
			auto readMembers = Reserve<string>( size );
			auto readIds = Reserve<UA_ReadValueId>( size );
			for( let& c : _query.Columns ){
				if( auto p = _attributes.find(c.JsonName); p != _attributes.end() ){
					readIds.push_back( UA_ReadValueId{nodeId, p->second, UA_STRING_NULL, { 0, UA_STRING_NULL }} );
					ids.push_back(p->second);
					readMembers.push_back( p->first );
				}
			}
			if( readIds.empty() )
				return Resume( move(jnode) );

			UA_ReadRequest request{
				.nodesToReadSize = readIds.size(),
				.nodesToRead = readIds.data()
			};
			ReadResponse response{ UA_Client_Service_read(*_client, request), _client->Handle(), nodeId, _sl };
			for( uint i=0; i<std::min(readMembers.size(), (uint)response.resultsSize); ++i ){
				Variant value = response.results[i].status ? Variant{ response.results[i].status } : Variant{ response.results[i].value };
				BREAK_IF( i==2 );
				jnode[readMembers[i]] = value.ToJson( true );
			}
			Resume( _query.ReturnRaw ? jnode : jobject{{"node", jnode}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}