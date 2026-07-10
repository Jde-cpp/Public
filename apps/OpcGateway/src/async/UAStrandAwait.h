#pragma once
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	//Runs a synchronous UA_Client service on the client's strand and resumes the awaiter with the result.
	//Sync services (translateBrowsePathsToNodeIds, getRemoteDataTypes, ...) internally drive the client's event
	//loop until the response arrives, so while one runs that client's strand is held (its data-change
	//notifications stall for the round-trip); other clients are unaffected. Convert the call to open62541's
	//async machinery if that stall ever matters.
	Τ struct UAStrandAwait final : TAwait<T>{
		using base = TAwait<T>;
		UAStrandAwait( sp<UAClient> client, function<T()> f, SRCE )ι: base{sl}, _client{move(client)}, _f{move(f)}{}
		α Suspend()ι->void override{
			_client->PostUA( [this]{
				try{
					this->Resume( _f() );
				}
				catch( exception& e ){
					this->ResumeExp( move(e) );
				}
			});
		}
	private:
		sp<UAClient> _client;
		function<T()> _f;
	};
}
