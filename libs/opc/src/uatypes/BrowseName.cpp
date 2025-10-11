#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/UAString.h>

namespace Jde::Opc{
	BrowseName::BrowseName( const jobject& j )ι:
		BrowseName( 0, Json::FindNumber<NsIndex>(j, "ns").value_or(0), Json::AsString(j, "name") )
	{}

	BrowseName::BrowseName( BrowseNamePK pk, NsIndex ns, sv name )ι:
		UA_QualifiedName{ ns, AllocUAString(name) },
		PK{ pk }
	{}
	BrowseName::BrowseName( sv fqBrowseName, NsIndex defaultNs )ε{
		auto pos = fqBrowseName.find('~');
		namespaceIndex = pos==string::npos ? defaultNs : To<NsIndex>( fqBrowseName.substr(0,pos) );
		name = AllocUAString( pos==string::npos ? fqBrowseName : fqBrowseName.substr( pos+1 ) );
	}
	BrowseName::BrowseName( UA_QualifiedName&& rhs )ι:
		UA_QualifiedName{ rhs },
		PK{}
	{}
	α BrowseName::ToJson( UA_QualifiedName ua )ι->jobject{
		jobject o;
		if( ua.name.length ){
			o["ns"] = ua.namespaceIndex;
			o["name"] = Opc::ToString( ua.name );
		}
		return o;
	}
	α BrowseName::ToJson()Ι->jobject{
		auto y = ToJson( *this );
		if( PK )
			y["pk"] = PK;
		return y;
	}
}