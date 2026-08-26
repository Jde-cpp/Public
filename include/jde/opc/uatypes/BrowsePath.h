#pragma once
#include "NodeId.h"

namespace Jde::Opc{
	//A browse path below the Objects folder - "<ns>~<name>/<ns>~<name>".  A segment's ns is a numeric index, an alias
	//from `nsAliases` (a namespace uri's runtime index - it moves with the nodeset load order, so never a literal in
	//config), or `defaultNs` when omitted.  Legs are HierarchicalReferences with subtypes: Organizes and HasComponent both walk.
	struct BrowsePath final : UA_BrowsePath, noncopyable{
		BrowsePath( sv path, NsIndex defaultNs, const flat_map<string,NsIndex>& nsAliases={}, SRCE )ε;
		BrowsePath( BrowsePath&& x )ι:UA_BrowsePath{ x }{ UA_BrowsePath_init( &x ); }
		~BrowsePath(){ UA_BrowsePath_clear( this ); }
		α ToString()Ι->string;//"<ns>~<name>/..." with the indexes the segments resolved to.
		//Server-side TranslateBrowsePathsToNodeIds - the first target; throws naming the path when there is none.
		Ω Resolve( UA_Server& server, sv path, NsIndex defaultNs, const flat_map<string,NsIndex>& nsAliases={}, SRCE )ε->NodeId;
	};
	//`uri`'s index in the server's namespace array; throws when no loaded nodeset declares it.
	α NamespaceIndex( UA_Server& server, sv uri, SRCE )ε->NsIndex;
}
