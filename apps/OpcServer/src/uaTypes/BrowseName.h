#pragma once

namespace Jde::Opc::Server{

	struct BrowseName final{
		BrowseName()ι = default;
		BrowseName( const jobject& j )ι;
		BrowseName( BrowseNamePK pk, NsIndex ns=0, string name={} )ι;

		operator UA_QualifiedName()Ι{ return UA_QualifiedName{ Ns, ToUV(Name) }; }
		α ToJson()Ι->jobject;
		α ToString()Ι->string{ return serialize( ToJson() ); }

		BrowseNamePK PK{};
		NsIndex Ns{};
		string Name;
	};
}