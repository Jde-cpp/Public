#include <jde/opc/uatypes/BrowseName.h>

namespace Jde::Opc{
	BrowseName::BrowseName( const jobject& j )ι:
		Ns{ Json::FindNumber<NsIndex>(j, "ns").value_or(0) },
		Name{ Json::AsString(j, "name") }
	{}
	BrowseName::BrowseName( BrowseNamePK pk, NsIndex ns, string name )ι:
		PK{ pk },
		Ns{ ns },
		Name{ move(name) }
	{}
	BrowseName::BrowseName( sv fqBrowseName, NsIndex defaultNs )ε{
		auto pos = fqBrowseName.find('~');
		Ns = pos==string::npos ? defaultNs : To<NsIndex>( fqBrowseName.substr(0,pos) );
		Name = pos==string::npos ? string{ fqBrowseName } : fqBrowseName.substr( pos+1 );
	}

	α BrowseName::ToJson()Ι->jobject{
		jobject o;
		if( PK )
			o["pk"] = PK;
		if( !Name.empty() ){
			o["ns"] = Ns;
			o["name"] = Name;
		}
		return o;
	}
}