#pragma once
#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include "NodeAwait.h"
#include "ObjectAttrAwait.h"
#include "ObjectTypeAttrAwait.h"
#include "ReferenceAwait.h"
#include "VariableAttrAwait.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Opc::Server {
	struct UAServer;
	struct ServerConfigAwait final : VoidAwait<> {
		ServerConfigAwait( SRCE )ι:	VoidAwait<>(sl){
			_nodes.reserve(1024);
			_nodes.emplace( 0, Node{0, {}} );
		};
		Ω ServerWhereClause( const DB::View& snTable, string alias )ε->DB::WhereClause;
	private:
		α Suspend()ι->void override { Execute(); }
		α Execute()ι->NodeAwait::Task;
		α LoadObjectAttrs()ι->ObjectAttrAwait::Task;
		α LoadObjectTypeAttr()ι->ObjectTypeAttrAwait::Task;
		α LoadReferences()ι->ReferenceAwait::Task;
		α LoadVAttrs()ι->VariableAttrAwait::Task;
		α SaveSystem()ι->DB::ExecuteAwait::Task;
		α Set()ι->void;
		string _serverName;
		flat_map<NodePK, Node> _nodes;
		flat_map<OAttrPK, ObjectAttr> _objectAttrs;
		flat_map<NodePK, Reference> _refs;
		flat_map<OTypeAttrPK, ObjectTypeAttr> _typeAttribs;
		flat_map<VAttrPK, VariableAttr> _vAttrs;
	};
}