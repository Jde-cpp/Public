#pragma once

/*
struct ReferenceDescription : UA_ReferenceDescription
{
	ReferenceDescription( UA_ReferenceDescription&& x )ι:UA_ReferenceDescription{ x }{ Zero( x ); }
	ReferenceDescription( ReferenceDescription&& x )ι:UA_ReferenceDescription{ *this }{ Zero( *this ); }
	~ReferenceDescription(){ UA_ReferenceDescription_clear(this); }

	α ToJson()ε->json;
};
*/