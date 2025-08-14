#include "ConnectAwait.h"
#include "../UAClient.h"

#define let const auto

namespace Jde::Opc::Gateway{
	flat_map<ServerCnnctnNK,flat_map<Credential,vector<ConnectAwait::Handle>>> _requests; mutex _requestMutex;

	α ConnectAwait::EraseRequests( str opcNK, Credential cred, lg& )ι->vector<ConnectAwait::Handle>{
		vector<ConnectAwait::Handle> handles;
		if( auto p = _requests.find(opcNK); p != _requests.end() ){
			if( auto q = p->second.find(cred); q != p->second.end() ){
				handles = move( q->second );
				p->second.erase( q );
				if( p->second.empty() )
					_requests.erase( p );
			}
		}
		return handles;
	}

	α ConnectAwait::Suspend()ι->void{
		if( auto pClient = UAClient::Find(_opcTarget, _cred); pClient )
			base::Resume( move(pClient) );
		else{
			_requestMutex.lock();
			auto opcHandles = _requests.try_emplace( _opcTarget ).first;
			auto credHandles = opcHandles->second.try_emplace( _cred ).first;
			credHandles->second.push_back( _h );
			_requestMutex.unlock();
			Create();
		}
	}
	α ConnectAwait::Create()ι->TAwait<vector<ServerCnnctn>>::Task{
		try{
			auto servers = co_await ServerCnnctnAwait{ _opcTarget };
			THROW_IF( servers.empty(), "Could not find opc server:  '{}'", _opcTarget );
			auto client = ms<UAClient>( move(servers.front()), _cred );
			client->Connect();
		}
		catch( const IException& e ){
			let ua = dynamic_cast<const UAException*>( &e );
			lg l{ _requestMutex };
			auto handles = EraseRequests( _opcTarget, _cred, l );
			for( auto& h : handles ){
				if( ua )
					h.promise().ResumeExp( UAException{*ua}, h );
				else
					h.promise().ResumeExp( Exception{e.what(), e.Code, e.Level(), e.Stack().front()}, h );
			}
		}
	}
	α ConnectAwait::Resume( sp<UAClient> pClient, str opcNK, Credential cred, function<void(ConnectAwait::Handle)> resume )ι->void{
		ASSERT( pClient );
		vector<ConnectAwait::Handle> handles;
		{
			lg l{ _requestMutex };
			handles = EraseRequests( opcNK, cred, l );
		}
		for( auto h : handles )
			resume( h );
	}

	α ConnectAwait::Resume( sp<UAClient> client, str opcNK, Credential cred )ι->void{
		Resume( client, opcNK, cred, [=](ConnectAwait::Handle h)mutable{ h.promise().Resume(move(client), h); } );
	}
	α ConnectAwait::Resume( sp<UAClient> client, str opcNK, Credential cred, const UAClientException&& e )ι->void{
		Resume( move(client), opcNK, cred, [sc=e.Code](ConnectAwait::Handle h){ h.promise().ResumeExp(UAClientException{(StatusCode)sc}, h); } );
	}
}