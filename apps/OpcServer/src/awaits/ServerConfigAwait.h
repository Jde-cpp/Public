#pragma once
#include <jde/db/awaits/SelectAwait.h>
#include <jde/db/awaits/ExecuteAwait.h>
#include "BrowseNameAwait.h"
#include "NodeAwait.h"
#include "ObjectAwait.h"
#include "ObjectTypeAwait.h"
#include "ReferenceAwait.h"
#include "VariableAwait.h"
#include "ConstructorAwait.h"
#include "../UAServer.h"

namespace Jde::DB{ struct AppSchema; }
namespace Jde::Opc::Server {
	struct UAServer;
	struct ServerConfigAwait final : VoidAwait<>{
		ServerConfigAwait( SRCE )ι:	VoidAwait<>(sl){
			GetUAServer()._dataTypes.reserve(1024);
		};
		Ω ServerWhereClause( const DB::View& snTable, string alias="n", SRCE )ε->DB::WhereClause;
	private:
		α Suspend()ι->void override { LoadBrowseNames(); }
		α LoadBrowseNames()ι->BrowseNameAwait::Task;
		α LoadConstructors()ι->ConstructorAwait::Task;
		α LoadObjectTypes()ι->ObjectTypeAwait::Task;
		α LoadObjects()ι->ObjectAwait::Task;
		α LoadReferences()ι->ReferenceAwait::Task;
		α LoadVariables()ι->VariableAwait::Task;
		α AllocateNodes()ι->NodeAwait::Task;
		α SaveSystem()ι->TAwait<NodePK>::Task;
		α Set()ι->void;
	};
}