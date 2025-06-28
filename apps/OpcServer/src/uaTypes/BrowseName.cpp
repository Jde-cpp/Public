#include "BrowseName.h"

namespace Jde::Opc::Server{
	BrowseName::BrowseName( const jobject& j )ι:
		Ns{ Json::FindNumber<NsIndex>(j, "ns").value_or(0) },
		Name{ Json::AsString(j, "name") }
	{}
	BrowseName::BrowseName( BrowseNamePK pk, NsIndex ns, string name )ι:
		PK{ pk },
		Ns{ ns },
		Name{ move(name) }
	{}

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