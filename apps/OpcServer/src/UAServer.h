#pragma once
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "UAConfig.h"
#include "uaTypes/Node.h"
#include "uaTypes/ObjectAttr.h"
#include "uaTypes/Object.h"
#include "uaTypes/ObjectType.h"
#include "uaTypes/Reference.h"
#include "uaTypes/Variable.h"

namespace Jde::Opc::Server {
	struct UAServer{
		UAServer()ε;
		~UAServer();

		Ω Constructor( UA_Server *server, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *typeId, void *typeContext, const UA_NodeId *nodeId, void **nodeContext )->UA_StatusCode;
		α Run()ι->void;
		α ConstructorValues( const NodeId& nodeId )ε->const flat_map<BrowseNamePK, Variant>&;

		α Load( fs::path configFile, SRCE )ε->void;
		α Find( NodePK parentPK, BrowseNamePK browsePK )Ι->const Node*;
		α Find( const Node& parent, const BrowseName& browse )Ι->const Node*;
		α FindBrowse( BrowseName& browse )Ι->bool;
		α GetBrowse( BrowseName& browse, SRCE )Ε->void;
		α GetBrowse( BrowseNamePK pk, SRCE )Ε->const BrowseName&;
		α FindDataType( NodePK nodePK )Ι->const UA_DataType*;
		α GetDataType( NodePK nodePK, SRCE )ε->const UA_DataType&;
		α GetParent( NodePK pk, SRCE )ε->Node&;
		α FindObjectishNode( NodePK pk )Ε->const Node*;
		α GetObjectish( NodePK pk, SRCE )Ε->const Node&;
		α GetObject( const NodeId& id, SRCE )ε->const Object&;
		α GetRefType( NodePK pk, SRCE )ε->NodeId&;
		α GetTypeDef( const NodeId& id, SRCE )ε->sp<ObjectType>;
		α GetTypeDef( NodePK pk, SRCE )ε->sp<ObjectType>;
		α GetVariable( NodePK pk, SRCE )ε->const Variable&;
		α Namespaces()ι->flat_map<uint,string>;
		α AddConstructor( UA_NodeId nodeId )ε->void;
		α AddConstructor( NodeId nodeId, flat_map<BrowseNamePK, Variant>&& values )ε->void;
		α AddObject( Object object, SRCE )ε->Object;
		α AddObjectType( sp<ObjectType> node, SRCE )ε->void;
		α AddReference( NodePK nodePK, const Reference& ref, SRCE )ε->void;
		α AddVariable( Variable variable, SRCE )->Variable;
		string ServerName;
	private:
		UAConfig _config;
		UA_Server* _ua{};
		optional<std::jthread> _thread;

		flat_map<BrowseNamePK, BrowseName> _browseNames;
		flat_map<NodeId, flat_map<BrowseNamePK, Variant>> _constructors;
		mutable flat_map<NodePK, UA_DataType*> _dataTypes;
		flat_map<NodePK, Object> _objects;
		flat_map<NodePK, sp<ObjectType>> _typeDefs; //ObjectTypes and VariableTypes
		flat_map<NodePK, Reference> _refs;
		flat_map<NodePK, NodeId> _refTypes;
		flat_map<VariablePK, Variable> _variables;
		volatile UA_Boolean _running{};

		friend struct ServerConfigAwait; friend struct OpcServerQL; friend struct BrowseNameAwait; friend struct ObjectQLAwait; friend struct ObjectTypeQLAwait; friend struct NodeAwait; friend struct VariableInsertAwait;
	};
}