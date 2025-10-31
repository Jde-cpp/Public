#pragma once

namespace Jde::Opc{
	struct BrowseName final : UA_QualifiedName{
		BrowseName()ι = default;
		BrowseName( const jobject& j )ι;
		BrowseName( BrowseNamePK pk, NsIndex ns=0, sv name={} )ι;
		BrowseName( sv fqBrowseName, NsIndex defaultNs )ε;
		BrowseName( UA_QualifiedName&& qn )ι;
		α operator==( const UA_QualifiedName& x )Ι->bool{ return namespaceIndex==x.namespaceIndex && ToSV(name)==ToSV(x.name); }

		Ω ToJson( UA_QualifiedName ua )ι->jobject;
		α ToJson()Ι->jobject;
		α ToString()Ι->string{ return serialize( ToJson() ); }

		BrowseNamePK PK{};
	};
}