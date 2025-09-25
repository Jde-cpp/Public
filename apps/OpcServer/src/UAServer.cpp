#include "UAServer.h"
#include <jde/opc/uatypes/helpers.h>
#include "UAAccess.h"
#include <NodesetLoader/backendOpen62541.h>

#define let const auto
namespace Jde::Opc::Server {
	constexpr ELogTags _tags{ ELogTags::App };
	UAServer::UAServer()ε:
		ServerName{ Settings::FindString("/opcServer/name").value_or("OpcServer") },
		_ua{ UA_Server_newWithConfig(&_config) }
	{}
	UAServer::~UAServer(){
		Information{ ELogTags::App, "Stopping OPC UA server..." };
		if( _thread.has_value() ){
			_running = false;
			_thread->request_stop();
			_thread->join();
			_thread.reset();
		}
		if( _ua ){
			UA_Server_delete( _ua );
			_ua = nullptr;
		}
	}
	//
	α UAServer::Run()ι->void{
		if( !_thread ){
			_thread = std::jthread{[this](std::stop_token /*st*/){
				SetThreadDscrptn( "UAServer" );
				_running = true;
				UA_Server_run( _ua, &_running );
				UA_Server_run_shutdown( _ua );
				UA_Server_delete( _ua );
				_ua = nullptr;
				Information{ ELogTags::App, "OPC UA server stopped." };
			}};
		}
	}
	α UAServer::Load( fs::path configFile, SL sl )ε->void{
		Information{ ELogTags::App, "Loading configuration from: '{}'", configFile.string() };
		CHECK_PATH( configFile, sl );
		if( !NodesetLoader_loadFile(_ua, configFile.string().c_str(), nullptr) )
			throw Exception( sl, "Failed to load nodeset file: '{}'", configFile.string() );
	}

	α UAServer::Constructor(UA_Server* /*server*/,
	                    const UA_NodeId* /*sessionId*/, void* /*sessionContext*/,
	                    const UA_NodeId* typeId, void* /*typeContext*/,
	                    const UA_NodeId* nodeId, void** /*nodeContext*/)->UA_StatusCode{
		auto& ua = GetUAServer();
		try{
			for( let& [pk, variant] : ua.ConstructorValues(NodeId{*typeId}) ){
				UA_RelativePathElement rpe;
				UA_RelativePathElement_init(&rpe);
				rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
				rpe.isInverse = false;
				rpe.includeSubtypes = false;
				//rpe.targetName = ua.GetBrowse( pk );
				rpe.targetName = UA_QualifiedName{1, "status"_uv};

				UA_BrowsePath bp{};
		    UA_BrowsePath_init(&bp);
				bp.startingNode = *nodeId;
				bp.relativePath.elementsSize = 1;
				bp.relativePath.elements = &rpe;

				auto bpr = UA_Server_translateBrowsePathToNodeIds( ua._ua, &bp );
				UAε( bpr.statusCode );
				THROW_IF( !bpr.targetsSize, "No targets found for node: {}, path: ({}){}", NodeId{*nodeId}.ToString(), rpe.targetName.namespaceIndex, ToString(rpe.targetName.name) );
				UAε( UA_Server_writeValue(ua._ua, bpr.targets[0].targetId.nodeId, variant) );
				UA_BrowsePathResult_clear(&bpr);
			}
			return UA_STATUSCODE_GOOD;
		}catch( exception& e ){
			Exception{ move(e) };
		}
		return UA_STATUSCODE_BAD;
	}
	α UAServer::AddConstructor( UA_NodeId nodeId )ε->void{
		DBG( "Adding constructor for node: '{}'", NodeId{nodeId}.ToString() );
		UAε( UA_Server_setNodeTypeLifecycle(_ua, move(nodeId), UA_NodeTypeLifecycle{UAServer::Constructor, nullptr}) );
	}
	α UAServer::AddConstructor( NodeId nodeId, flat_map<BrowseNamePK, Variant>&& values )ε->void{
		let type = GetTypeDef( nodeId );
		DBG( "Adding constructor for type: '{}'", type->ToString() );
		_constructors.try_emplace( nodeId, move(values) );
		AddConstructor( move(nodeId) );
	}
	α UAServer::ConstructorValues( const NodeId& nodeId )ε->const flat_map<BrowseNamePK, Variant>&{
		let p = _constructors.find( nodeId );
		THROW_IF( p==_constructors.end(), "Constructor values not found for node: '{}'", nodeId.ToString() );
		return p->second;
	}

	α UAServer::AddObject( Object object, SL sl )ε->Object{
		let& requestedNodeId = object;
		auto& parent = GetParent( object.ParentNodePK, sl );
		UA_NodeId id{};
		let status = UA_Server_addObjectNode(
			_ua,
			requestedNodeId,
			parent,
			GetRefType( object.ReferenceTypePK, sl ),
			object.Browse,
			object.TypeDef ? *object.TypeDef : UA_NODEID_NULL,
			object,
			(void*)object.PK,
			&id
		);
		THROW_IFSL( status, "({})Failed to add object node: error:'{}', node:'{}'", Ƒ("{:x}", status), UA_StatusCode_name(status), object.ToString() );
		dynamic_cast<NodeId&>(object) = id;
		DBGSL( "Added Object: {}", object.ToString(parent) );
		return object.PK ? _objects.try_emplace( object.PK, move(object) ).first->second : object;
	}

	α UAServer::AddObjectType( sp<ObjectType> oType, SL sl )ε->void{
		UA_NodeId id;
		auto& parent = GetParent( oType->ParentNodePK, sl );
		UAε( UA_Server_addObjectTypeNode(
			_ua,
			*oType,
			parent,
			GetRefType( oType->ReferenceTypePK, sl ),
			oType->Browse,
			*oType,
			(void*)oType->PK, &id
		) );
		dynamic_cast<NodeId&>(*oType) = id;
		DBGSL( "Added ObjectType: {}", oType->ToString(parent) );
		if( oType->PK )
			_typeDefs.try_emplace( oType->PK, oType ).first->second;
	}
	α UAServer::AddReference( NodePK nodePK, const Reference& ref, SL sl )ε->void{
		auto& source = GetVariable( ref.SourcePK );
		UAε( UA_Server_addReference(
			_ua,
			ref.SourcePK ? source : UA_NODEID_NULL,
			GetRefType( ref.RefTypePK, sl ),
			ref.TargetPK ? (UA_ExpandedNodeId)ExNodeId{ (NodeId&)GetObject(NodeId{(uint32_t)ref.TargetPK}, sl) } : UA_EXPANDEDNODEID_NULL,
			ref.IsForward
		) );
		_refs.try_emplace( nodePK, ref );
		auto& sourceParent = GetParent( source.ParentNodePK, sl );
		DBGSL( "Added Reference: {} -> {} ({})", source.ToString(sourceParent), ref.TargetPK, ref.RefTypePK );
	}
	α UAServer::AddVariable( Variable variable, SL sl )->Variable{
		ASSERT( variable.TypeDef );
		UA_NodeId id;
		auto& parent = GetParent( variable.ParentNodePK, sl );
		let status = UA_Server_addVariableNode(
			_ua,
			variable,
			parent,
			GetRefType( variable.ReferenceTypePK, sl ),
			variable.Browse,
			*variable.TypeDef,
			variable,
			(void*)variable.PK,
			&id);
		THROW_IFX( status, UAException(status, variable.ToString(), sl, {ELogLevel::Error, EOpcLogTags::Opc}) );
		dynamic_cast<NodeId&>(variable) = id;
		DBGSL( "Added Variable: {}", variable.ToString(parent) );
		return variable.PK ? _variables.try_emplace( variable.PK, move(variable) ).first->second : variable;
	}


	α UAServer::Find( NodePK parentPK, BrowseNamePK browsePK )Ι->const Node*{
		auto f = [=]( let& kv )->bool{ return kv.second.ParentNodePK == parentPK && kv.second.Browse.PK == browsePK; };
		auto spF = [=]( let& kv )->bool{ return kv.second->ParentNodePK == parentPK && kv.second->Browse.PK == browsePK; };
		if( auto p = find_if(_objects, f); p!=_objects.end() )
			return &p->second;
		if( auto p = find_if(_typeDefs, spF); p!=_typeDefs.end() )
			return p->second.get();
		return {};
	}
	α UAServer::Find( const Node& parent, const BrowseName& browse )Ι->const Node*{
		const Node* y{};
		for( const auto& [pk, node] : _objects ){
			if( node.ParentNodePK != parent.PK )
				continue;
			if( let nodeBrowse = _browseNames.find( node.Browse.PK );
				nodeBrowse==_browseNames.end() || nodeBrowse->second.Ns!=browse.Ns || nodeBrowse->second.Name!=browse.Name ){
				continue;
			}
			y = &node;
			break;
		}
		return y;
	}

	α UAServer::FindBrowse( BrowseName& browse )Ι->bool{
		let p = browse.PK
			? _browseNames.find( browse.PK )
			: find_if(_browseNames, [&browse](const auto& kv){ return kv.second.Ns==browse.Ns && kv.second.Name==browse.Name; });
		if( p!=_browseNames.end() )
			browse = p->second;
		return p!=_browseNames.end();
	}
	α UAServer::GetBrowse( BrowseNamePK pk, SL sl )Ε->const BrowseName&{
		let p = _browseNames.find( pk );
		THROW_IFSL( p==_browseNames.end(), "({})BrowseName not found", pk );
		return p->second;
	}

	α UAServer::GetBrowse( BrowseName& browse, SL sl )Ε->void{
		THROW_IFSL( !FindBrowse(browse), "BrowseName not found: {}", browse.ToString() );
	}

	α UAServer::FindDataType( NodePK nodePK )Ι->const UA_DataType*{
		auto p = _dataTypes.find( nodePK );
		if( p==_dataTypes.end() && nodePK<=32750 ){
			for( uint i=0; i<UA_TYPES_COUNT; ++i ){
				if( UA_TYPES[i].typeId.identifier.numeric==nodePK ){
					p = _dataTypes.try_emplace( nodePK, &UA_TYPES[i] ).first;
					break;
				}
			}
		}
		return p==_dataTypes.end() ? nullptr : p->second;
	}
	α UAServer::GetDataType( NodePK nodePK, SL sl )ε->const UA_DataType&{
		auto p = FindDataType( nodePK );
		THROW_IFSL( p==nullptr, "({:x})Data type not found", nodePK );
		return *p;
	}
	α UAServer::GetParent( NodePK pk, SL sl )ε->Node&{
		if( let p = _objects.find(pk); p!=_objects.end() )
			return p->second;
		if( let p = _typeDefs.find(pk); p!=_typeDefs.end() )
			return *p->second;
		if( let p = _variables.find(pk); p!=_variables.end() )
			return p->second;
		throw Exception{ sl, "({:x})Parent node not found", pk };
	}

	α UAServer::GetObjectish( NodePK pk, SL sl )Ε->const Node&{
		if( let p = _objects.find(pk); p!=_objects.end() )
			return p->second;
		if( let p = _typeDefs.find(pk); p!=_typeDefs.end() )
			return *p->second;
		throw Exception{ sl, "[{:x}]Object[Type] node not found", pk };
	}
	α UAServer::GetObject( const NodeId& id, SL sl )ε->const Object&{
		let p = find_if( _objects, [&]( let& kv ){return kv.second==id;} );
		THROW_IFSL( p==_objects.end(), "Object not found: {}", id.ToString() );
		return p->second;
	}
	α UAServer::GetRefType( NodePK pk, SL sl )ε->NodeId&{
		auto p = _refTypes.find(pk);
		if( p==_refTypes.end() && pk<=32750 )
			p = _refTypes.try_emplace( pk, pk ).first;
		THROW_IFSL( p==_refTypes.end(), "({:x})Reference type not found", pk );
		return p->second;
	}
	α UAServer::GetTypeDef( const NodeId& id, SL sl )ε->sp<ObjectType>{
		let p = find_if( _typeDefs, [&]( let& kv ){return *kv.second==id;} );
		THROW_IFSL( p==_typeDefs.end(), "Object type not found: {}", id.ToString() );
		return p->second;
	}
	α UAServer::GetTypeDef( NodePK pk, SL sl )ε->sp<ObjectType>{
		auto p = _typeDefs.find( pk );
		if( p==_typeDefs.end() && pk<=32750 )
			p = _typeDefs.try_emplace( pk, ms<ObjectType>(UA_NodeId{0, UA_NODEIDTYPE_NUMERIC, (UA_UInt32)pk}) ).first;
		THROW_IFSL( p==_typeDefs.end(), "({})Object type not found", Ƒ("{:x}", pk) );
		return p->second;
	}
	α UAServer::GetVariable( NodePK pk, SL sl )ε->const Variable&{
		auto p = _variables.find(pk);
		THROW_IFSL( p==_variables.end(), "({})Variable not found", Ƒ("{:x}", pk) );
		return p->second;
	}
}