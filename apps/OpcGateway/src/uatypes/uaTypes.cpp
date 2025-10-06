#include "uaTypes.h"
#include <jde/fwk/utils/collections.h>
#include <jde/ql/types/TableQL.h>

#define let const auto
namespace Jde::Opc::Gateway{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::Opc };
	BrowsePathsToNodeIdResponse::BrowsePathsToNodeIdResponse( UA_TranslateBrowsePathsToNodeIdsResponse&& rhs, const vector<string>& paths, Handle uaHandle, SL sl )ε:
		UA_TranslateBrowsePathsToNodeIdsResponse{ move(rhs) },
		_paths{ paths },
		_sl{ sl }{
		THROW_IFX( responseHeader.serviceResult, UAClientException(responseHeader.serviceResult, uaHandle, Ƒ("UA_Client_Service_translateBrowsePathsToNodeIds('{}').", Path()), sl) );
		THROW_IFX( resultsSize!=paths.size(), UAClientException(responseHeader.serviceResult, uaHandle, Ƒ("UA_Client_Service_translateBrowsePathsToNodeIds resultSize: {}, pathSize: {}", resultsSize, paths.size()), sl) );
	}

	BrowsePathsToNodeIdResponse::operator flat_map<string,Expected>()Ε{
		auto y = ReserveMap<string,Expected>( resultsSize );
		for( uint i=0; i<resultsSize; ++i ){
			optional<Expected> expected;
			if( results[i].statusCode )
				expected = Expected{ std::unexpected{results[i].statusCode} };
			else if( results[i].targetsSize==0 )
				expected = Expected{ std::unexpected{UA_STATUSCODE_BADNOTFOUND} };
			else{
				if( results[i].targetsSize>1 )
					LOGSL( ELogLevel::Warning, _sl, _tags, "Multiple targets found for '{}'. Using first.", _paths[i] );
				expected = Expected{ ExNodeId{results[i].targets[0].targetId} };
			}
			y.emplace( _paths[i], move(*expected) );
		}
		return y;
	}

	flat_map<string,UA_AttributeId> _attributes{
		{"invalid", UA_ATTRIBUTEID_INVALID},
		{"description", UA_ATTRIBUTEID_DESCRIPTION},
		{"value", UA_ATTRIBUTEID_VALUE},
		{"browseName", UA_ATTRIBUTEID_BROWSENAME},
		{"name", UA_ATTRIBUTEID_DISPLAYNAME},
		{"dataType", UA_ATTRIBUTEID_DATATYPE}
	};

	ReadRequest::ReadRequest( const QL::TableQL& ql, const string& reqPath, const flat_map<string, std::expected<ExNodeId,StatusCode>>& pathNodes, jobject& jReqNode, flat_map<NodeId, jobject>& parents )ι:
		UA_ReadRequest{}{
		auto parentsQL = ql.FindTable("parents");
		//auto ids = Reserve<UA_AttributeId>( size );
		//auto readMembers = Reserve<string>( size );
		_readIds.reserve( ql.Columns.size() + (parentsQL ? parentsQL->Columns.size()*pathNodes.size() : 0) );
		//_readMembers.reserve( _readIds.capacity() );
		auto addAttribs = [&]( const QL::TableQL& ql, const ExNodeId& nodeId, sv path, jobject& jNode ){
			if( ql.FindColumn("path") )
				jNode["path"] = path;
			else if( ql.FindColumn("eid") )
				nodeId.Add( jNode );
			if( ql.FindColumn("id") )
				NodeId{nodeId.nodeId}.Add( jNode );

			for( let& c : ql.Columns ){
				if( auto attrib = _attributes.find(c.JsonName); attrib != _attributes.end() ){
					_readIds.push_back( UA_ReadValueId{nodeId.nodeId, (UA_UInt32)attrib->second, UA_STRING_NULL, {0, UA_STRING_NULL}} );
					//_readMembers.push_back( {nodeId.value(), attrib->second} );
					//ids.push_back(attrib->second);
					//readMembers.push_back( attrib->first );
				}
			}
		};
		if( auto req = pathNodes.find(reqPath); req != pathNodes.end() ){
			if( !req->second )
				jReqNode = UAException::ToJson( req->second.error() );
			else
				addAttribs( ql, req->second.value(), reqPath, jReqNode );
		}
		if( parentsQL ){
			jarray* errors{};
			for( let& pathNode : pathNodes ){
				if( pathNode.first == reqPath )
					continue;
				if( pathNode.second ){
					NodeId parentNodeId{ pathNode.second.value().nodeId };
					addAttribs( *parentsQL, parentNodeId, pathNode.first, parents[parentNodeId] );
				}
				else{
					if( !errors ){
						jReqNode["errors"] = jarray{};
						errors = (jarray*)jReqNode.if_contains( "errors" );
					}
					auto error = UAException::ToJson( pathNode.second.error() );
					error["path"] = pathNode.first;
					errors->emplace_back( move(error) );
				}
			}
		}

		nodesToReadSize=_readIds.size();
		nodesToRead=_readIds.data();
	}
	α ReadRequest::AtribString( UA_AttributeId id )->const string&{
		for( let& [k,v] : _attributes )
			if( v==id )
				return k;
		return _attributes.begin()->first;
	}

	ReadResponse::ReadResponse( UA_ReadResponse&& rhs, Handle uaHandle, SL sl )ε:
		UA_ReadResponse{ move(rhs) }{
		THROW_IFX( rhs.responseHeader.serviceResult, UAClientException(rhs.responseHeader.serviceResult, uaHandle, "UA_Client_Service_read().", sl) );
	}
}