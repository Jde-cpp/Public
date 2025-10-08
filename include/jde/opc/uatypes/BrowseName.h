#pragma once

namespace Jde::Opc{
	struct BrowseName final{
		BrowseName()ι = default;
		BrowseName( const jobject& j )ι;
		BrowseName( BrowseNamePK pk, NsIndex ns=0, string name={} )ι;
		BrowseName( sv fqBrowseName, NsIndex defaultNs )ε;
		α operator==( const UA_QualifiedName& x )Ι->bool{ return Ns==x.namespaceIndex && Name==ToSV(x.name); }

		operator UA_QualifiedName()Ι{ return UA_QualifiedName{ Ns, ToUV(Name) }; }
		α ToJson()Ι->jobject;
		α ToString()Ι->string{ return serialize( ToJson() ); }

		BrowseNamePK PK{};
		NsIndex Ns{};
		string Name;
	};
}