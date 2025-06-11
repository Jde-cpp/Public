#include "ServerConfigAwait.h"
#include <jde/db/generators/Coalesce.h>
#include <jde/db/generators/InsertClause.h>
#include <jde/db/generators/Syntax.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/IDataSource.h>
#include <jde/opc/uatypes/Node.h>
#include <jde/opc/uatypes/UAException.h>
#include "../UAServer.h"
#include "../uaTypes/ObjectAttr.h"


#define let const auto
namespace Jde::Opc::Server{
	α ServerConfigAwait::ServerWhereClause( const DB::View& snTable, string alias )ε->DB::WhereClause{
		DB::Value serverName{ServerName()};
		return {{ {DB::AliasCol{alias, snTable.GetColumnPtr("deleted")}, DB::EOperator::Equal, DB::Value{} },
					  { DB::Coalesce{DB::AliasCol{alias, snTable.GetColumnPtr("server_name")}, serverName}, DB::EOperator::Equal, serverName }
				}};
	}
	α ServerConfigAwait::Execute()ι->NodeAwait::Task{
		try{
			_nodes = co_await NodeAwait{};
			SaveSystem();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ServerConfigAwait::SaveSystem()ι->DB::ExecuteAwait::Task{
		try{
			for( uint i=0; i<UA_TYPES_COUNT; ++i ){
				//BREAK;//find out what to do about binary encoding, fill in rest of data_type.
				NodeId extNodeId{ DataType(i).typeId };
				let& nodeId = extNodeId.nodeId;
				ASSERT( nodeId.namespaceIndex==0 && nodeId.identifierType==UA_NODEIDTYPE_NUMERIC );
				if( _nodes.contains(nodeId.identifier.numeric) )
					continue;
				let table = GetViewPtr( "node_ids" );
				DB::InsertClause proc{
					table->InsertProcName(),
					{ {nodeId.namespaceIndex},
						nodeId.identifierType==UA_NODEIDTYPE_NUMERIC ? DB::Value{nodeId.identifier.numeric} : DB::Value{},
						nodeId.identifierType==UA_NODEIDTYPE_STRING ? DB::Value{ToString(nodeId.identifier.string)} : DB::Value{},
						nodeId.identifierType==UA_NODEIDTYPE_GUID ? DB::Value{ToGuid(nodeId.identifier.guid)} : DB::Value{},
						nodeId.identifierType==UA_NODEIDTYPE_BYTESTRING ? DB::Value{FromByteString(nodeId.identifier.byteString)} : DB::Value{},
						DB::Value{}, // namespaceUri
						DB::Value{}, // serverIndex
						DB::Value{} // isGlobal
					},
				};
				let nodePK = co_await DS().ScalerAsync<NodePK>( proc.Move() );
				_nodes.try_emplace( nodePK, nodePK, move(extNodeId) );
			}
			LoadObjectAttrs();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::LoadObjectAttrs()ι->ObjectAttrAwait::Task{
		try{
			_objectAttrs = co_await ObjectAttrAwait{};
			LoadObjectTypeAttr();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::LoadObjectTypeAttr()ι->ObjectTypeAttrAwait::Task{
		try{
			_typeAttribs = co_await ObjectTypeAttrAwait{};
			LoadReferences();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ServerConfigAwait::LoadReferences()ι->ReferenceAwait::Task{
		try{
			_refs = co_await ReferenceAwait{};
			LoadVAttrs();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ServerConfigAwait::LoadVAttrs()ι->VariableAttrAwait::Task{
		try{
			_vAttrs = co_await VariableAttrAwait{};
			Set();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::Set()ι->void{
		let ua = GetUAServer()._ua;
		flat_set<NodePK> done;
		function<void(const Node&)> process = [&]( const Node& node ){

			let parent = _nodes.at( node.ParentNodeId.value_or(0) );
			if( parent.ExNode.nodeId.namespaceIndex!=0 && !done.contains(*node.ParentNodeId) )
				process( parent );

			let referenceType = _nodes.at( node.ReferenceTypeId.value_or(0) );
			if( referenceType.ExNode.nodeId.namespaceIndex!=0 && !done.contains(*node.ReferenceTypeId) )
				process( referenceType );

			let typeDef = _nodes.at( node.TypeDefId.value_or(0) );
			if( typeDef.ExNode.nodeId.namespaceIndex!=0 && !done.contains(*node.TypeDefId) )
				process( typeDef );

			if( node.OAttributeId ){
				UA_Server_addObjectNode(
					ua,
					node.ExNode.nodeId,
					parent.ExNode.nodeId,
					referenceType.ExNode.nodeId,
					UA_QualifiedName{1, ToUV(node.Name)},//TODO find out what to dow with ns.
					typeDef.ExNode.nodeId,
					_objectAttrs.at(node.OAttributeId),
					(void*)node.NodeId, nullptr
				);
			}
			else if( node.VAttributeId ){
				UA_Server_addVariableNode(
					ua,
					node.ExNode.nodeId,
					parent.ExNode.nodeId,
					referenceType.ExNode.nodeId,
					UA_QualifiedName{1, ToUV(node.Name)},
					typeDef.ExNode.nodeId,
					_vAttrs.at(node.VAttributeId),
					(void*)node.NodeId, nullptr
				);
			}
			else if( node.TypeAttribId ){
				UA_Server_addObjectTypeNode(
					ua,
					node.ExNode.nodeId,
					parent.ExNode.nodeId,
					referenceType.ExNode.nodeId,
					UA_QualifiedName{ 1, ToUV(node.Name) },
					_typeAttribs.at( node.TypeAttribId ),
					(void*)node.NodeId, nullptr
				);
			}
			if( let ref = _refs.contains(node.NodeId) ? _refs.at(node.NodeId) : optional<Reference>{}; ref ){
				let& target = _nodes.at( ref->TargetPK );
				let type = _nodes.at( ref->TypePK );
				if( !done.contains(ref->TargetPK) )
					process( target );
				if( !done.contains(ref->TypePK) )
					process( type );
				UA_Server_addReference(
					ua,
					node.ExNode.nodeId,
					type.ExNode.nodeId,
					target.ExNode,
					ref->IsForward ? UA_TRUE : UA_FALSE
				);
			}
			done.insert( node.NodeId );
		};
		for( auto&&	[nodeId,node] : _nodes ){
			if( !done.contains(nodeId) && node.ExNode.nodeId.namespaceIndex!=0 )
				process( node );
		}
		Resume();
		// _ua._server = UA_Server_new();
		// UA_ServerConfig config = getConfiguration();
		// UA_StatusCode status = UA_Server_run_startup(_ua._server, &config);
		// if( status!=UA_STATUSCODE_GOOD )
		// 	throw Exception{ SRCE, "Failed to start UA_Server: {}", UA_StatusCode_name(status) };
	}
}
