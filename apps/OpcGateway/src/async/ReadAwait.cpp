#include "ReadAwait.h"
#include <jde/fwk/utils/collections.h>
#include <jde/ql/types/TableQL.h>
#include <jde/opc/uatypes/Variant.h>
#include "../UAClient.h"
#include "../uatypes/Browse.h"

#define let const auto
namespace Jde::Opc::Gateway{
	flat_map<string,UA_AttributeId> _attributes{
    {"invalid", UA_ATTRIBUTEID_INVALID},
//		{"id", UA_ATTRIBUTEID_NODEID},
		{"accessLevel", UA_ATTRIBUTEID_ACCESSLEVEL},
		{"accessLevelEx", UA_ATTRIBUTEID_ACCESSLEVELEX},
		{"accessRestrictions", UA_ATTRIBUTEID_ACCESSRESTRICTIONS},
		{"arrayDimensions", UA_ATTRIBUTEID_ARRAYDIMENSIONS},
    {"browse", UA_ATTRIBUTEID_BROWSENAME},
		{"containsNoLoops", UA_ATTRIBUTEID_CONTAINSNOLOOPS},
    {"dataType", UA_ATTRIBUTEID_DATATYPE},
		{"dataTypeDefinition", UA_ATTRIBUTEID_DATATYPEDEFINITION},
    {"description", UA_ATTRIBUTEID_DESCRIPTION},
		{"eventNotifier", UA_ATTRIBUTEID_EVENTNOTIFIER},
    {"executable", UA_ATTRIBUTEID_EXECUTABLE},
		{"historizing", UA_ATTRIBUTEID_HISTORIZING},
		{"inverseName", UA_ATTRIBUTEID_INVERSENAME},
		{"isAbstract", UA_ATTRIBUTEID_ISABSTRACT},
		{"minimumSamplingInterval", UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL},
    {"name", UA_ATTRIBUTEID_DISPLAYNAME},
		{"nodeClass", UA_ATTRIBUTEID_NODECLASS},
		{"rolePermissions", UA_ATTRIBUTEID_ROLEPERMISSIONS},
		{"userAccessLevel", UA_ATTRIBUTEID_USERACCESSLEVEL},
		{"userExecutable", UA_ATTRIBUTEID_USEREXECUTABLE},
		{"userRolePermissions", UA_ATTRIBUTEID_USERROLEPERMISSIONS},
		{"userWriteMask", UA_ATTRIBUTEID_USERWRITEMASK},
		{"writeMask", UA_ATTRIBUTEID_WRITEMASK},
		{"symmetric", UA_ATTRIBUTEID_SYMMETRIC},
    {"value", UA_ATTRIBUTEID_VALUE},
		{"valueRank", UA_ATTRIBUTEID_VALUERANK},
	};

	Ω attributes( const QL::TableQL& ql )ι->flat_map<UA_NodeClass, vector<UA_AttributeId>>{
		auto y = ReserveMap<UA_NodeClass, vector<UA_AttributeId>>( ql.Columns.size() );
		auto extract = [&]( const QL::TableQL& ql, UA_NodeClass nodeClass ){
			vector<UA_AttributeId> attribs;
			for( let& c : ql.Columns ){
				if( auto attrib = _attributes.find(c.JsonName); attrib != _attributes.end() )
					attribs.emplace_back( attrib->second );
			}
			if( !attribs.empty() )
				 y[nodeClass] = move(attribs);
		};
		extract( ql, UA_NODECLASS_UNSPECIFIED );
		for( auto frag : ql.InlineFragments ){
			let& name = frag.JsonName;
			if( name=="Object" )
				extract( frag, UA_NODECLASS_OBJECT );
			else if( name=="Variable" )
				extract( frag, UA_NODECLASS_VARIABLE );
			else if( name=="Method" )
				extract( frag, UA_NODECLASS_METHOD );
			else if( name=="ObjectType" )
				extract( frag, UA_NODECLASS_OBJECTTYPE );
			else if( name=="VariableType" )
				extract( frag, UA_NODECLASS_VARIABLETYPE );
			else if( name=="RefType" )
				extract( frag, UA_NODECLASS_REFERENCETYPE );
			else if( name=="DataType" )
				extract( frag, UA_NODECLASS_DATATYPE );
			else if( name=="View" )
				extract( frag, UA_NODECLASS_VIEW );
		}
		return y;
	}
	Ω allAttributes( const QL::TableQL& ql )ι->vector<UA_AttributeId>{
		auto y = attributes( ql );
		auto all = y.find( UA_NODECLASS_UNSPECIFIED );
		return all==y.end() ? vector<UA_AttributeId>{} : all->second;
	}
	ReadRequest::ReadRequest( Browse::Response&& browse, QL::TableQL&& ql )ι:
		UA_ReadRequest{}{
		let attribs = attributes( ql );
		if( attribs.empty() )
			return;
		auto all = attribs.find( UA_NODECLASS_UNSPECIFIED );
		_readIds.reserve( (all==attribs.end() ? 0 : all->second.size())*(browse.resultsSize ? browse.results[0].referencesSize : 0) );

		auto add = [&]( const UA_ReferenceDescription& ref, const vector<UA_AttributeId>& attribs ){
			for( let& attrib : attribs )
				_readIds.push_back( UA_ReadValueId{ref.nodeId.nodeId, (UA_UInt32)attrib, UA_STRING_NULL, {0, UA_STRING_NULL}} );
		};
		browse.VisitWhile( 0, [&]( let& ref ){
			if( all!=attribs.end() )
				add( ref, all->second );
			if( auto p = attribs.find( ref.nodeClass ); p != attribs.end() )
				add( ref, p->second );
			return true;
		});
		nodesToReadSize=_readIds.size();
		nodesToRead=_readIds.data();
	}
	ReadRequest::ReadRequest( const NodeId& nodeId, const QL::TableQL& ql )ι:
		UA_ReadRequest{}{
		let attribs = allAttributes( ql );
		for( let& attrib : attribs )
			_readIds.push_back( UA_ReadValueId{nodeId, (UA_UInt32)attrib, UA_STRING_NULL, {0, UA_STRING_NULL}} );
		nodesToReadSize=_readIds.size();
		nodesToRead=_readIds.data();
	}
	ReadRequest::ReadRequest( const NodeId& nodeId, UA_AttributeId attrib )ι:
		UA_ReadRequest{}{
		_readIds.push_back( UA_ReadValueId{nodeId, (UA_UInt32)attrib, UA_STRING_NULL, {0, UA_STRING_NULL}} );
		nodesToReadSize=_readIds.size();
		nodesToRead=_readIds.data();
	}

	α ReadRequest::Add( const QL::TableQL& ql, const flat_map<NodeId, jobject>& nodes )ι->void{
		let attribs = allAttributes( ql );
		for( let& [nodeId, _] : nodes ){
			for( let& attrib : attribs )
				_readIds.push_back( UA_ReadValueId{nodeId, (UA_UInt32)attrib, UA_STRING_NULL, {0, UA_STRING_NULL}} );
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

	ReadResponse::ReadResponse( UA_ReadResponse&& x )ι:
		UA_ReadResponse{ x }{
		UA_ReadResponse_init( &x );
	}
	ReadResponse::ReadResponse( ReadResponse&& x )ι:
		UA_ReadResponse{ x },
		Request{ move(x.Request) }{
		UA_ReadResponse_init( &x );
	}

	α ReadResponse::operator=( ReadResponse&& x )ι->ReadResponse&{
		if( this != &x ){
			UA_ReadResponse_clear(this);
			*(UA_ReadResponse*)this = x;
			Request = move(x.Request);
			UA_ReadResponse_init( &x );
		}
		return *this;
	}

	α ReadResponse::Validate( Handle uahandle, SL sl )ε->void{
		THROW_IFX( responseHeader.serviceResult, UAClientException(responseHeader.serviceResult, uahandle, "UA_Client_Service_read().", sl) );
	}
		// Request{ move(request) }{
		// THROW_IFX( rhs.responseHeader.serviceResult, UAClientException(rhs.responseHeader.serviceResult, uaHandle, "UA_Client_Service_read().", sl) );
	α ReadResponse::ScalerJson()ι->jobject{
		jobject j;
			for( uint i=0; i<std::min(Request->nodesToReadSize, (uint)resultsSize); ++i ){
			let result = results[0];
			UA_ReadValueId& attribReq = Request->nodesToRead[i];
			Variant value = result.status ? Variant{ result.status } : Variant{ move(result.value) };
			j[ReadRequest::AtribString((UA_AttributeId)attribReq.attributeId)] = move(value).ToJson( true );
		}
		return j;
	}
	α ReadResponse::SetJson( flat_map<NodeId, jobject>& nodes )ι->void{
		for( uint i=0; i<std::min(Request->nodesToReadSize, (uint)resultsSize); ++i ){
			UA_ReadValueId& attribReq = Request->nodesToRead[i];
			auto result = results[i];
			const NodeId nodeId{ attribReq.nodeId };
			auto nodeIt = nodes.try_emplace( nodeId );
			jobject& j = nodeIt.first->second;
			Variant value = result.status ? Variant{ result.status } : Variant{ move(result.value) };
			let attrib = (UA_AttributeId)attribReq.attributeId;
			j[ReadRequest::AtribString(attrib)] = move(value).ToJson( true );
		}
	}

	α ReadResponse::ScalerDataType()ι->UA_DataType*{
		UA_DataType* dataType{};
		if( auto value = resultsSize && results[0].hasValue ? &results[0].value : nullptr; value && value->type && value->type==&UA_TYPES[UA_TYPES_NODEID] ){
			NodeId nodeId{ *(UA_NodeId*)results[0].value.data };
			for( uint i=0; !dataType && i<UA_TYPES_COUNT; ++i ){
				if( NodeId{UA_TYPES[i].typeId}==nodeId )
					dataType = &UA_TYPES[i];
			}
		}
		return dataType;
	}

	α ReadAwait::Suspend()ι->void{
		UA_Client_sendAsyncReadRequest( *_client, &_request, ReadAwait::OnResponse, &_h, &_requestId );
		_client->Process( _requestId, "read" );
	}
	α ReadAwait::await_resume()ε->ReadResponse{
		if( !Promise() )
			return {};
		ASSERT( Promise()->Emplaced() );
		_client->ClearRequest( _requestId );
		ReadResponse result{ std::move(*Promise()->Value()) };
		result.Validate( _client->Handle(), _sl );
		result.Request = move(_request);
		return result;
	}
	α ReadAwait::OnResponse( UA_Client* /*client*/, void* hptr, UA_UInt32 /*requestId*/, UA_ReadResponse* rr )ι->void{
		ASSERT( hptr );
		auto& h = *(Handle*)hptr;
		h.promise().Resume( ReadResponse{move(*rr)}, h );
	}
}