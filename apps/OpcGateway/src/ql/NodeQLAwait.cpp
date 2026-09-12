#include "NodeQLAwait.h"
#include <jde/fwk/co/AnyAwait.h>
#include <jde/fwk/utils/collections.h>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "../UAClient.h"
#include "../async/ConnectAwait.h"
#include "../async/ReadValueAwait.h"
#include "../async/UAStrandAwait.h"

#define let const auto
namespace Jde::Opc::Gateway{
	α NodeQLAwait::Execute()ι->TAwait<BrowsePathResponse>::Task{
		try{
			BrowsePathResponse pathNodes;
			NodeId nodeId;
			flat_map<NodeId, jobject> jParents;
			flat_map<NodeId, jobject> jChildren;
			auto parentsQL{ _query.FindTable("parents") };
			if( auto nodePath = _query.FindPtr<jstring>("path"); nodePath ){
				pathNodes = co_await UAStrandAwait<BrowsePathResponse>{ _client, [this, nodePath, parentsQL]{ return _client->BrowsePathsToNodeIds(*nodePath, parentsQL!=nullptr); }, _sl };//sync UA service - must run on the client's strand.
				const bool savePath{ parentsQL && parentsQL->FindColumn("path") };
				for( auto node = pathNodes.begin(); node != pathNodes.end(); ++node ){
					if( node->second.has_value() ){
						nodeId = node->second->nodeId;
						if( savePath )
							jParents.try_emplace( node->second->nodeId, jobject{{"path", node->first}} );
						continue;
					}
					else if( node==pathNodes.begin() )
						ResumeExp( UAClientException{node->second.error(), _client->Handle(), {}, _sl} );
					else
						Browse( move(pathNodes), std::prev(node)->first, parentsQL, move(jParents) );
					co_return;
				}
				jParents.erase( nodeId );//the loop leaves nodeId as the deepest (full-path) node; the remainder of jParents are its parents.
				AddAttributes( move(nodeId), parentsQL, move(jParents) );
			}
			else if( auto children = _query.FindTable("children"); children ){
				Browse( NodeId{_query}, move(*children) );
				co_return;
			}
			else if( _query.FindColumn("path") )
				Path( NodeId::ParseQL(_query) );
			else
				AddAttributes( NodeId::ParseQL(_query) );
		}
		catch( runtime_error& e ){
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
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α NodeQLAwait::AddAttributes( Browse::Response&& browseResp, QL::TableQL&& childrenQL, flat_map<NodeId, jobject> jChildren )ι->TAwait<ReadResponse>::Task{
		try{
			auto resp = co_await ReadAwait{ move(browseResp), move(childrenQL), _client };
			resp.SetJson( jChildren );
			jarray a;
			for( auto&& [id, j] : jChildren )
				a.emplace_back( move(j) );
			jobject jReqNode{ {"children", move(a)} };
			Resume( _query.ReturnRaw ? jReqNode : jobject{{"node", jReqNode}} );
		}
		catch( runtime_error& e ){
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
				let found = !response.VisitWhile( 0, [&](const UA_ReferenceDescription& ref){
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
			catch( runtime_error& e ){
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
				if( auto p = parents.find(nodeId->nodeId); p!=parents.end() )
					parents.erase( p );
				ReadRequest request{ {nodeId->nodeId}, _query };
				if( parentQL )
					request.Add( *parentQL, parents );
				auto resp = co_await ReadAwait{ move(request), _client };
			 	auto json = resp.GetJson();
				if( auto p = json.find(nodeId->nodeId); p!=json.end() )
					jReqNode = move( p->second );
				for( auto&& [id, jparent] : parents ){
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
				for( auto&& [id, jparent] : parents ){
					if( parentQL->FindColumn("id") )
						id.Add( jparent );
					jparents.emplace_back( move(jparent) );
				}
				jReqNode["parents"] = move( jparents );
			}
			Resume( _query.ReturnRaw ? jReqNode : jobject{{"node", jReqNode}} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	//The path form above answers a path with an id;  this is the reverse, for a node known only by id - the node-scoped
	//resources access holds (`criteria` = "ns=5;i=5005") link to the node's page, which is addressed by browse path.  Walk
	//up:  each inverse HierarchicalReferences browse names the parent, until the Objects folder - a node outside that tree
	//(types, views, the Root) has no page and answers null, as does one whose browse name holds a '/' (the url's own
	//separator - the search index skips those too).  Segments are spelled as the index and BrowsePathsToNodeIds spell them:
	//the bare browse name in the connection's default namespace, `ns~name` otherwise.  A node's own name is not in an
	//inverse result (those carry the parents'), so it is one attribute read - through Any(), this coroutine's own type is
	//the browse's.  Several parents (an object Organized by a folder and a Component of another):  the Objects folder wins
	//when it is one of them, else the first - the tree the crawl reaches the node by.
	α NodeQLAwait::Path( vector<NodeId> nodeIds )ι->TAwait<Browse::Response>::Task{
		try{
			const NodeId objects = NodeId::ObjectsFolder();
			let defaultNs = _client->DefaultBrowseNs();
			auto segment = [defaultNs]( const UA_QualifiedName& browse )->optional<string>{
				string name{ ToSV(browse.name) };
				if( name.find('/')!=string::npos )
					return {};
				return browse.namespaceIndex==defaultNs ? name : Ƒ( "{}~{}", browse.namespaceIndex, name );
			};
			for( let& requested : nodeIds ){
				jvalue path{ nullptr };
				vector<string> segments;
				auto own = co_await Any( ReadAwait{ReadRequest{requested, UA_ATTRIBUTEID_BROWSENAME}, _client} );
				optional<string> ownSegment;
				if( own.resultsSize && own.results[0].status==UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&own.results[0].value, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]) )
					ownSegment = segment( *(const UA_QualifiedName*)own.results[0].value.data );
				if( ownSegment )
					segments.push_back( *ownSegment );
				NodeId current{ requested };
				for( uint depth=0; ownSegment; ++depth ){
					if( UA_NodeId_equal(&current, &objects) ){
						segments.pop_back();//the walk started at Objects itself:  its own name is not a segment.
						path = Str::Join( segments, "/" );
						break;
					}
					if( depth>=64 ){//a reference cycle - the walk would never reach Objects.
						WARNT( BrowseTag, "[{}]path: '{}' is deeper than 64 or cyclic - no path.", hex(_client->Handle()), requested.ToString() );
						break;
					}
					auto response = co_await Browse::FoldersAwait{ Browse::Request::Parents(NodeId{current}), _client, _sl };
					optional<NodeId> parent; optional<string> parentSegment;
					response.VisitWhile( 0, [&]( const UA_ReferenceDescription& ref ){
						NodeId id{ ref.nodeId.nodeId };
						let isObjects = UA_NodeId_equal( &id, &objects );
						if( !parent || isObjects ){
							parent = id;
							parentSegment = segment( ref.browseName );
						}
						return !isObjects;
					} );
					if( !parent )
						break;//no hierarchical parent, and not Objects - outside the tree the pages address.
					if( UA_NodeId_equal(&*parent, &objects) ){
						path = Str::Join( segments, "/" );
						break;
					}
					if( !parentSegment )
						break;//an unroutable parent name.
					segments.insert( segments.begin(), *parentSegment );
					current = *parent;
				}
				_paths.emplace( NodeId{requested}, move(path) );
			}
			AddAttributes( move(nodeIds) );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
	α NodeQLAwait::AddAttributes( vector<NodeId> nodeIds )ι->TAwait<ReadResponse>::Task{
		try{
			jvalue json;
			ReadRequest request{ nodeIds, _query };
			if( _paths.size() ){//Path's rows:  one per requested id even when `path` is the only column (nothing to read, and a Read of nothing is an error).
				flat_map<NodeId, jobject> nodes;
				if( request.nodesToReadSize ){
					auto resp = co_await ReadAwait{ move(request), _client };
					nodes = resp.GetJson();
				}
				jarray rows;
				for( let& id : nodeIds ){
					auto p = nodes.find( id );
					jobject row = p==nodes.end() ? jobject{} : move( p->second );
					if( _query.FindColumn("id") )
						row["id"] = id.ToJson();
					auto path = _paths.find( id );
					row["path"] = path==_paths.end() ? jvalue{nullptr} : path->second;
					rows.push_back( move(row) );
				}
				json = _query.IsPlural() ? jvalue{ move(rows) } : ( rows.size() ? jvalue{move(rows[0])} : jobject{} );
			}
			else{
				auto resp = co_await ReadAwait{ move(request), _client };
				json = resp.ToJson( _query );
			}
			Resume( _query.ReturnRaw ? move(json) : jobject{{"nodes", move(json)}} );
		}
		catch( runtime_error& e ){
			ResumeExp( move(e) );
		}
	}
}