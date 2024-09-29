#include <jde/iot/async/ConnectAwait.h>
#include <jde/iot/uatypes/UAClient.h>

#define var const auto

namespace Jde::Iot
{
	flat_map<tuple<string,string>,vector<HCoroutine>> _requests; mutex _requestMutex;

	α ConnectAwait::Suspend()ι->void{
		if( auto pClient = UAClient::Find(_id, _userId, _password); pClient )
			Jde::Resume( move(pClient), _h );
		else{
			auto key = make_tuple( _id,_userId );
			_requestMutex.lock();
			if( auto p = _requests.find( key ); p!=_requests.end() ){
				p->second.push_back( _h );
				_requestMutex.unlock();
			}
			else{
				_requests[key] = {_h};
				_requestMutex.unlock();
				Create( _id, _userId, _password );
			}
		}
	}
	α ConnectAwait::Create( string opcServerId, string userId, string password )ι->Task{
		try{
			auto pServer = ( co_await OpcServer::Select(opcServerId) ).UP<OpcServer>(); THROW_IF( !pServer, "Could not find opc server:  '{}'", opcServerId );
			auto pClient = ms<UAClient>( move(*pServer), userId, password );
			pClient->Connect();
		}
		catch( const IException& e ){
			lg _{ _requestMutex };
			var ua = dynamic_cast<const UAException*>( &e );
			auto key = make_tuple( move(opcServerId), move(userId) );
			for( auto& h : _requests[key] ){
				if( ua )
					Jde::Resume( UAException{*ua}, move(h) );
				else
					Jde::Resume( Exception{e.what(), e.Code, e.Level(), e.Stack().front()}, move(h) );
			}
			_requests.erase( key );
		}
	}
	α ConnectAwait::Resume( sp<UAClient> pClient, str target, str userId, function<void(HCoroutine&&)> resume )ι->void{
		ASSERT( pClient );
		vector<HCoroutine> handles;
		{
			lg _{ _requestMutex };
			auto key = make_tuple( target, userId );
			if( auto p = _requests.find( key ); p!=_requests.end() ){
				handles = move( _requests[key] );
				_requests.erase( key );
			}
		}
		for( auto h : handles )
			resume( move(h) );
	}

	α ConnectAwait::Resume( sp<UAClient>&& pClient, str target, str userId )ι->void{
		Resume( pClient, target, userId, [p=pClient](HCoroutine&& h){ Jde::Resume((sp<UAClient>)move(p), move(h)); } );
	}
	α ConnectAwait::Resume( sp<UAClient>&& pClient, str target, str userId, const UAException&& e )ι->void{
		Resume( move(pClient), target, userId, [sc=e.Code](HCoroutine&& h){ Jde::Resume(UAException{(StatusCode)sc}, move(h)); } );
	}
}