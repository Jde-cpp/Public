#include "ConnectAwait.h"
#include "../UAClient.h"

#define let const auto

namespace Jde::Opc::Gateway{
	flat_map<ServerCnnctnNK,flat_map<Credential,vector<ConnectAwait::Handle>>> _requests; mutex _requestMutex;
	α credential( SessionPK sessionId, UserPK user, str opc )ι->optional<Credential>{
		optional<Credential> cred;
		if( sessionId ){
			cred = GetCredential( sessionId, opc );
			if( !cred && user ) //if user/pwd would have cred, otherwise use jwt
				cred = Credential{ Ƒ("{:x}", sessionId) };
		}
		return cred;
	}
	ConnectAwait::ConnectAwait( string&& opc, SessionPK sessionId, UserPK user, SL sl )ι:
		base{sl},
		_opcTarget{ move(opc) },
		_cred{ credential(sessionId, user, _opcTarget).value_or(Credential{}) }
	{}

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
		if( auto client = UAClient::Find(_opcTarget, _cred); client ){
			TRACET( ((ELogTags)EOpcLogTags::Opc) | ELogTags::Access, "[{:x}]Found client for cred: {}", client->Handle(), _cred.ToString() );
			base::Resume( move(client) );
		}
		else{
			_requestMutex.lock();
			auto opcHandles = _requests.try_emplace( _opcTarget ).first;
			auto credHandles = opcHandles->second.try_emplace( _cred ).first;
			credHandles->second.push_back( _h );
			let size = credHandles->second.size();
			_requestMutex.unlock();
			if( size == 1 )
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
		catch( const exception& e ){
			let ua = dynamic_cast<const UAClientException*>( &e );
			lg l{ _requestMutex };
			auto handles = EraseRequests( _opcTarget, _cred, l );
			for( auto& h : handles ){
				if( ua )
					h.promise().ResumeExp( UAClientException(*ua), h );
				else
					h.promise().ResumeExp( Exception{e.what(), 0, ELogLevel::Error, SRCE_CUR}, h );
			}
		}
	}
	α ConnectAwait::Resume( str opcNK, Credential cred, function<void(ConnectAwait::Handle)> resume )ι->void{
		vector<ConnectAwait::Handle> handles;
		{
			lg l{ _requestMutex };
			handles = EraseRequests( opcNK, cred, l );
		}
		for( auto h : handles )
			resume( h );
	}

	α ConnectAwait::Resume( sp<UAClient> client )ι->void{
		Resume( client->Target(), client->Credential, [client](ConnectAwait::Handle h){ h.promise().Resume(sp<UAClient>(client), h); } );
	}
	α ConnectAwait::Resume( str opcNK, Credential cred, const UAClientException&& e )ι->void{
		Resume( opcNK, cred, [e2=move(e)](ConnectAwait::Handle h)mutable{ h.promise().ResumeExp(move(e2), h); } );
	}
}