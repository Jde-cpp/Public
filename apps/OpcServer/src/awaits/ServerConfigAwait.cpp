#include "ServerConfigAwait.h"
#include <jde/db/generators/Functions.h>
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
		constexpr std::array<UA_UInt16,5> _objectPKs{ UA_NS0ID_MODELLINGRULE_MANDATORY, UA_NS0ID_OBJECTSFOLDER, UA_NS0ID_TYPESFOLDER, UA_NS0ID_SERVER, UA_NS0ID_SERVERCONFIGURATION };
		constexpr std::array<UA_UInt16,6> _refPKs{ UA_NS0ID_ORGANIZES, UA_NS0ID_HASCOMPONENT, UA_NS0ID_HASPROPERTY, UA_NS0ID_HASMODELLINGRULE, UA_NS0ID_HASEVENTSOURCE, UA_NS0ID_HASSUBTYPE };
		constexpr std::array<UA_UInt16,2> _objectTypePKs{ UA_NS0ID_BASEOBJECTTYPE, UA_NS0ID_FOLDERTYPE };
		constexpr std::array<UA_UInt16,2> _variableTypePKs{ UA_NS0ID_BASEDATAVARIABLETYPE, UA_NS0ID_BASEVARIABLETYPE };

	α ServerConfigAwait::ServerWhereClause( const DB::View& snTable, string alias, bool includeDeleted, SL sl )ε->DB::WhereClause{
		DB::Value serverId{ServerId()};
		DB::WhereClause where{ DB::Coalesce{DB::AliasCol{alias, snTable.GetColumnPtr("server_id", sl)}, serverId}, DB::EOperator::Equal, serverId };
		// if( !includeDeleted )
		// 	where+={ DB::AliasCol{alias, snTable.GetColumnPtr("deleted", sl)}, DB::EOperator::Equal, DB::Value{} };
		return where;
	}

	α ServerConfigAwait::LoadBrowseNames()ι->BrowseNameAwait::Task{
		try{
			GetUAServer()._browseNames = co_await BrowseNameAwait{};
			LoadConstructors();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ServerConfigAwait::LoadConstructors()ι->ConstructorAwait::Task{
		try{
			GetUAServer()._constructors = co_await ConstructorAwait{};
			LoadObjectTypes();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::LoadObjectTypes()ι->ObjectTypeAwait::Task{
		try{
			GetUAServer()._typeDefs = co_await ObjectTypeAwait{};
			LoadObjects();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::LoadObjects()ι->ObjectAwait::Task{
		try{
			GetUAServer()._objects = co_await ObjectAwait{};
			LoadReferences();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::LoadReferences()ι->ReferenceAwait::Task{
		try{
			GetUAServer()._refs = co_await ReferenceAwait{};
			LoadVariables();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}
	α ServerConfigAwait::LoadVariables()ι->VariableAwait::Task{
		try{
			GetUAServer()._variables = co_await VariableAwait{};
			AllocateNodes();
			//SaveSystem( /*move(nodes)*/ );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::AllocateNodes()ι->NodeAwait::Task{
		try{
			auto nodes = co_await NodeAwait{};
			auto& ua = GetUAServer();
			for( auto&& [pk, node] : nodes ){
				bool found{ true };
				if( pk==0 || pk>32750 )
					continue;
				if( find(_refPKs, pk)!=_refPKs.end() )
				  ua._refTypes.try_emplace( pk, move(node.nodeId) );
				else if( find(_objectPKs, pk)!=_objectPKs.end() )
				  ua._objects.try_emplace( pk, node.nodeId );
				else if( find(_objectTypePKs, pk)!=_objectTypePKs.end() || find(_variableTypePKs,pk)!=_variableTypePKs.end() )
					ua._typeDefs.try_emplace( pk, ms<ObjectType>(node.nodeId) );
				else
					found = ua.FindDataType( pk );
				if( !found )
					Error{ ELogTags::App, "Unknown node type: {}", serialize(node.ToJson()) };
			}
			SaveSystem( /*move(nodes)*/ );
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::SaveSystem( /*flat_map<NodePK, NodeId> nodes*/ )ι->DB::ExecuteAwait::Task{
		let& table = GetView( "node_ids" );
		auto insertNodeIdClause = [&table]( const NodeId& nodeId )->DB::InsertClause {
			auto params = nodeId.InsertParams( true );
			params.emplace_back( DB::Value{} );//isGlobal
			return DB::InsertClause{
				table.InsertProcName(),
				move(params)
			};
		};
		UAServer& ua = GetUAServer();
		try{
			for( auto pk : _objectPKs ){
				if( ua._objects.contains(pk) )
					continue;
				NodeId id{pk};
				let nodePK = co_await DS().InsertSeq<NodePK>( insertNodeIdClause(id) );
				ua._objects.try_emplace( nodePK, id.nodeId );
			}
			for( auto pk : _refPKs ){
				if( ua._refTypes.contains(pk) )
					continue;
				NodeId node{pk};
				let nodePK = co_await DS().InsertSeq<NodePK>( insertNodeIdClause(node) );
				ua._refTypes.try_emplace( nodePK, move(node) );
			}
			for( auto pk : _objectTypePKs ){
				if( ua._typeDefs.contains(pk) )
					continue;
				NodeId node{pk};
				let nodePK = co_await DS().InsertSeq<NodePK>( insertNodeIdClause(node) );
				ua._typeDefs.try_emplace( nodePK, ms<ObjectType>(node.nodeId) );
			}
			for( auto pk : _variableTypePKs ){
				if( ua._typeDefs.contains(pk) )
					continue;
				NodeId node{pk};
				let nodePK = co_await DS().InsertSeq<NodePK>( insertNodeIdClause(node) );
				ua._variables.try_emplace( pk, node.nodeId );
			}
			Set();
		}
		catch( exception& e ){
			ResumeExp( move(e) );
		}
	}

	α ServerConfigAwait::Set()ι->void{
		auto& ua = GetUAServer();
		flat_set<NodePK> done;
		for( auto&&	[pk,node] : GetUAServer()._typeDefs ){
			if( done.contains(pk) || node->IsSystem() )
				continue;
			ua.AddObjectType( node );
			done.insert( pk );
			if( auto p = ua._constructors.find(pk); p!=ua._constructors.end() )
				ua.AddConstructor( node->nodeId );
			for( auto&& [pk,variable] : ua._variables ){
				if( variable.ParentNodePK==pk )
					ua.AddVariable( variable );
			}
		}
		for( auto&&	[pk,node] : GetUAServer()._objects ){
			if( done.contains(pk) || node.IsSystem() )
				continue;
			ua.AddObject( node );
			for( auto&& [pk,variable] : ua._variables ){
				if( variable.ParentNodePK==pk )
					ua.AddVariable( variable );
			}
		}
		for( auto&&	[pk,ref] : ua._refs ){
			UA_Server_addReference(
				ua._ua,
				ua.GetVariable(ref.SourcePK).nodeId,
				ua.GetRefType(ref.RefTypePK).nodeId,
				ua.GetObjectish(ref.TargetPK),
				ref.IsForward
			);
		}
		Resume();
	}
}
