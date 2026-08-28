#pragma once
#include <atomic>
#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/Variant.h>
#include "UAConfig.h"
#include "jde/fwk/process/process.h"
#include "uaTypes/Node.h"
#include "uaTypes/Object.h"
#include "uaTypes/ObjectType.h"
#include "uaTypes/Reference.h"
#include "uaTypes/Variable.h"
#include <thread>

namespace Jde::Opc::Server {
	struct UAServer final{
		UAServer()ε;
		~UAServer();
		operator UA_Server&()ι{ ASSERT(_ua); return *_ua; }
		Ω Constructor( UA_Server *server, const UA_NodeId *sessionId, void *sessionContext, const UA_NodeId *typeId, void *typeContext, const UA_NodeId *nodeId, void **nodeContext )->UA_StatusCode;
		α Run()ι->void;
		α ConstructorValues( const NodeId& nodeId )ε->const flat_map<BrowseNamePK, Variant>&;

		α Load( fs::path configFile, SRCE )ε->void;
		α PublishDataTypes()ι->void;
		α GetBrowse( BrowseNamePK pk, SRCE )Ε->const BrowseName&;
		α FindDataType( NodePK nodePK )Ι->const UA_DataType*;
		α GetDataType( NodePK nodePK, SRCE )ε->const UA_DataType&;
		α GetParent( NodePK pk, SRCE )ε->Node&;
		α GetRefType( NodePK pk, SRCE )ε->NodeId&;
		α GetTypeDef( NodePK pk, SRCE )ε->sp<ObjectType>;
		α Namespaces()ι->flat_map<uint,string>;
		α AddConstructor( UA_NodeId nodeId )ε->void;
		α AddObject( Object object, SRCE )ε->Object;
		α AddObjectType( sp<ObjectType> node, SRCE )ε->void;
		α AddReference( NodePK nodePK, const Reference& ref, SRCE )ε->void;
		α AddVariable( Variable variable, SRCE )->Variable;
		α Ptr()ι->UA_Server*{ return _ua; }
		string ServerName;
	private:
		UAConfig _config;
		UA_Server* _ua{};
		optional<std::jthread> _thread;
		vector<UA_DataTypeArray> _customTypes;//PublishDataTypes' mirror of the server-internal lists; entries point at open62541's arrays, so cleanup is false and this only has to outlive the server.

		flat_map<BrowseNamePK, BrowseName> _browseNames;
		flat_map<NodeId, flat_map<BrowseNamePK, Variant>> _constructors;
		mutable flat_map<NodePK, UA_DataType*> _dataTypes;
		flat_map<NodePK, Object> _objects;
		flat_map<NodePK, sp<ObjectType>> _typeDefs; //ObjectTypes and VariableTypes
		flat_map<NodePK, Reference> _refs;
		flat_map<NodePK, NodeId> _refTypes;
		flat_map<VariablePK, Variable> _variables;
		std::atomic<UA_Boolean> _running{};

		friend struct ServerConfigAwait;
	};
}