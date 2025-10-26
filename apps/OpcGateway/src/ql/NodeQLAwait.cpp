#include "NodeQLAwait.h"
#include <jde/fwk/utils/collections.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "../UAClient.h"
#include "../async/ConnectAwait.h"
#include "../async/ReadValueAwait.h"

#define let const auto
namespace Jde::Opc::Gateway{

	α NodeQLAwait::Execute()ι->TAwait<sp<UAClient>>::Task{
		auto opcId{ _query.FindPtr<jstring>("opc") };
		try{
			_client = co_await ConnectAwait{ opcId ? string{move(*opcId)} : string{}, _sessionPK, _executer, _sl };
			BrowsePathResponse pathNodes;
			NodeId nodeId;
			flat_map<NodeId, jobject> jParents;
			flat_map<NodeId, jobject> jChildren;
			auto parentsQL{ _query.FindTable("parents") };
			//auto childrenQL{ _query.FindTable("children") };
			if( auto nodePath{Json::FindString(_query.Args,"path").value_or(string{})}; nodePath.size() ){
				pathNodes = _client->BrowsePathsToNodeIds( nodePath, parentsQL!=nullptr );
				const bool savePath{ parentsQL && parentsQL->FindColumn("path") };
				for( auto node = pathNodes.begin(); node != pathNodes.end(); ++node ){
					if( node->second.has_value() ){
						nodeId = node->second->nodeId;
						if( savePath )
							jParents.try_emplace( node->second->nodeId, jobject{{"path", node->first}});
						continue;
					}
					else if( node==pathNodes.begin() )
						ResumeExp( UAClientException{node->second.error(), _client->Handle(), {}, _sl} );
					else
						Browse( move(pathNodes), std::prev(node)->first, parentsQL, move(jParents) );
					co_return;
				}
				auto lastNode = std::prev(jParents.end());
				nodeId = lastNode->first;
				jParents.erase( lastNode );
			}
			else if( auto children = _query.FindTable("children"); children ){
				Browse( NodeId{_query.Args}, move(*children) );
				co_return;
			}
			else{
				nodeId = NodeId{ _query.Args };
				pathNodes.emplace( string{}, ExNodeId{_query.Args} );
			}
			AddAttributes( move(nodeId), parentsQL, move(jParents) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α NodeQLAwait::Browse( NodeId parentId, QL::TableQL childrenQL )ι->TAwait<Browse::Response>::Task{
		try{
			flat_map<NodeId, jobject> jChildren;
			auto browseResp = co_await Browse::FoldersAwait{ parentId, childrenQL, _client, _sl };
			browseResp.SetJson( jChildren, childrenQL.FindColumn("id") );
			AddAttributes( move(browseResp), move(childrenQL), move(jChildren) );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α NodeQLAwait::AddAttributes( Browse::Response&& browseResp, QL::TableQL&& childrenQL, flat_map<NodeId, jobject> jChildren )ι->TAwait<ReadResponse>::Task{
		try{
			auto resp = co_await ReadAwait{ move(browseResp), move(childrenQL), _client };
			resp.SetJson( jChildren );
			jarray a;
			for( auto& [id, j] : jChildren )
				a.emplace_back( move(j) );
			jobject jReqNode{{"children", move(a)}};
			Resume( _query.ReturnRaw ? jReqNode : jobject{{"node", jReqNode}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α NodeQLAwait::Browse( BrowsePathResponse pathNodes, str lastGoodParent, QL::TableQL* parentsQL, flat_map<NodeId, jobject> jParents )ι->TAwait<Browse::Response>::Task{
		auto parent  = pathNodes.find( lastGoodParent );
		std::expected<ExNodeId,StatusCode> nodeId{ parent->second };
		for( auto pathNode = std::next(parent); nodeId.has_value() && pathNode!=pathNodes.end(); parent = pathNode, ++pathNode ){
			try{
				let response = co_await Browse::FoldersAwait{ parent->second.value().nodeId, UA_BROWSERESULTMASK_BROWSENAME, _client, _sl };
				const BrowseName reqBrowse{ Str::Split(pathNode->first,'/').back(), _client->DefaultBrowseNs() };
				const bool savePath{ parentsQL && parentsQL->FindColumn("path") };
				let found = !response.VisitWhile( 0, [&]( const UA_ReferenceDescription& ref ){
					let shouldContinue = reqBrowse!=ref.browseName;
					if( !shouldContinue ){
						nodeId = ExNodeId{ ref.nodeId };
						if( savePath )
							jParents.try_emplace( nodeId->nodeId, jobject{{"path", pathNode->first}} );
						pathNode->second = nodeId;
					}
					return shouldContinue;
				} );
				if( !found )
					nodeId = pathNode->second;
			}
			catch( exception& e ){
				ResumeExp( move(e) );
				co_return;
			}
		}
	 	AddAttributes( move(nodeId), parentsQL, move(jParents) );
	}

	α NodeQLAwait::AddAttributes( ExpectedNodeId nodeId, QL::TableQL* parentQL, flat_map<NodeId, jobject> parents )ι->TAwait<ReadResponse>::Task{
		try{
			jobject jReqNode;
			if( nodeId ){
				if( auto p = parents.find( nodeId->nodeId ); p!=parents.end() )
					parents.erase( p );
				ReadRequest request{ nodeId->nodeId, _query };
				if( parentQL )
					request.Add( *parentQL, parents );
				auto resp = co_await ReadAwait{ move(request), _client };
			 	auto json = resp.GetJson();
				if( auto p = json.find( nodeId->nodeId ); p!=json.end() )
					jReqNode = move(p->second);
				for( auto& [id, jparent] : parents ){
				 	if( auto p = json.find(id); p!=json.end() )
						jparent.insert( p->second.begin(), p->second.end() );
				}
				if( _query.FindColumn("id") )
					nodeId->Add( jReqNode );
				TRACET( ELogTags::Test, "jReqNode: {}", serialize(jReqNode) );
			}
			else
				jReqNode = UAException::ToJson( nodeId.error() );
			if( parentQL ){
				jarray jparents;
				for( auto& [id, jparent] : parents ){
					if( parentQL->FindColumn("id") )
						id.Add( jparent );
					jparents.emplace_back( move(jparent) );
				}
				jReqNode["parents"] = move(jparents);
			}
			Resume( _query.ReturnRaw ? jReqNode : jobject{{"node", jReqNode}} );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
}