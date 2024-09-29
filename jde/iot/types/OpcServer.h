#pragma once

namespace Jde::Iot{
	struct OpcServer{
		OpcServer( str address )ι:Url{address}{}
		OpcServer( const DB::IRow& r )ε;
		Ω Select()ι->AsyncAwait{ return Select(nullptr); };
		//returns up<vector<OpcServer>> if id=nullptr, otherwise up<OpcServer>.
		ΓI Ω Select( variant<nullptr_t,OpcPK,OpcNK> id, bool includeDeleted=false )ι->AsyncAwait;

		OpcPK Id;
		string Url;
		string CertificateUri;
		bool IsDefault;
		string Name;
		OpcNK Target;
	};
}