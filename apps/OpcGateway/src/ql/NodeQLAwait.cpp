#include "NodeQLAwait.h"
#include <jde/fwk/utils/collections.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "../UAClient.h"
#include "../async/ConnectAwait.h"
#include "../uatypes/uaTypes.h"


#define let const auto
namespace Jde::Opc::Gateway{

	α NodeQLAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		auto opcId = Json::FindString(_query.Args,"opc").value_or("");
		try{
			TRACET( ELogTags::Test, "opc: {}, UserPK: {:x}, SessionId: {:x}", opcId, _executer.Value, _sessionPK );
			_client = co_await ConnectAwait{ move(opcId), _sessionPK, _executer, _sl };
			flat_map<string, std::expected<ExNodeId,StatusCode>> pathNodes;

			auto parents = _query.FindTable("parents");
			auto reqPath = Json::FindString(_query.Args,"path").value_or( string{} );
			if( reqPath.size() ){
				pathNodes = _client->BrowsePathsToNodeIds( reqPath, parents );
				if( auto request = pathNodes.rbegin(); pathNodes.size()>1 && !request->second ){//request path failed.
					if( auto parent = std::next(request); parent->second.has_value() ){ //for some reason request path fails.
						Browse( parent->second.value().nodeId, move(pathNodes), parents, move(reqPath) );
						co_return;
					}
				}
			}
			else
				pathNodes.emplace( string{}, ExNodeId{_query.Args} );
			AddAttributes( move(pathNodes), parents, move(reqPath) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α NodeQLAwait::Browse( NodeId parentNodeId, flat_map<string, std::expected<ExNodeId,StatusCode>> pathNodes, QL::TableQL* parents, string reqPath )ι->TAwait<Browse::Response>::Task{
		auto response = co_await Browse::FoldersAwait{ parentNodeId, _client, _sl };
		const BrowseName browseName{ Str::Split(reqPath, '/').back(), _client->DefaultBrowseNs() };
		if( response.resultsSize ){
			for( auto ref : Iterable<UA_ReferenceDescription>(response.results[0].references, response.results[0].referencesSize) ){
				if( browseName==ref.browseName ){
					pathNodes.rbegin()->second = ExNodeId{ ref.nodeId };
					break;
				}
			}
		}
	 	AddAttributes( move(pathNodes), parents, move(reqPath) );
	}

	α NodeQLAwait::AddAttributes( flat_map<string, std::expected<ExNodeId,StatusCode>>&& pathNodes, QL::TableQL* parentQL, string reqPath )ι->void{
		try{
			jobject jReqNode;
			flat_map<NodeId, jobject> parents;
			ReadRequest request{ _query, reqPath, pathNodes, jReqNode, parents };
			ReadResponse response{ UA_Client_Service_read(*_client, request), _client->Handle(), _sl };
			for( uint i=0; i<std::min(request.nodesToReadSize, (uint)response.resultsSize); ++i ){
				UA_ReadValueId& attribReq = request.nodesToRead[i];
				const NodeId nodeId{ attribReq.nodeId };
				auto pathIt = find_if( pathNodes, [&nodeId](let& pn){return pn.second && NodeId{pn.second.value().nodeId}==nodeId;} );
				ASSERT( pathIt != pathNodes.end() );
				if( pathIt == pathNodes.end() )
					continue;
				let isParent = parentQL && pathIt->first.size() && pathIt->first != Json::FindSV(_query.Args,"path").value_or(sv{});
				Variant value = response.results[i].status ? Variant{ response.results[i].status } : Variant{ response.results[i].value };
				jobject& j = isParent ? parents[nodeId] : jReqNode;
				j[ReadRequest::AtribString((UA_AttributeId)attribReq.attributeId)] = value.ToJson( true );
			}
			if( parentQL ){
				jarray jparents;
				for( auto& [id, jparent] : parents )
					jparents.emplace_back( move(jparent) );
				jReqNode["parents"] = move(jparents);
			}
			Resume( _query.ReturnRaw ? jReqNode : jobject{{"node", jReqNode}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}