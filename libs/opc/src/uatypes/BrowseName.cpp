#include <jde/opc/uatypes/BrowseName.h>
#include <jde/opc/uatypes/UAString.h>

namespace Jde::Opc{
	BrowseName::BrowseName( const jobject& j )ε:
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
		PK{}{
		UA_QualifiedName_init( &rhs );
	}
	BrowseName::BrowseName( const BrowseName& x )ι:
		UA_QualifiedName{ x.namespaceIndex, UA_STRING_NULL },
		PK{ x.PK }{
		UA_String_copy( &x.name, &name );
	}
	BrowseName::BrowseName( BrowseName&& x )ι:
		UA_QualifiedName{ x },
		PK{ x.PK }{
		UA_QualifiedName_init( &x );
	}
	α BrowseName::operator=( const BrowseName& x )ι->BrowseName&{
		if( this!=&x ){
			UA_QualifiedName_clear( this );
			namespaceIndex = x.namespaceIndex;
			UA_String_copy( &x.name, &name );
			PK = x.PK;
		}
		return *this;
	}
	α BrowseName::operator=( BrowseName&& x )ι->BrowseName&{
		if( this!=&x ){
			UA_QualifiedName_clear( this );
			static_cast<UA_QualifiedName&>(*this) = x;
			PK = x.PK;
			UA_QualifiedName_init( &x );
		}
		return *this;
	}
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