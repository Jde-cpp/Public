#include <jde/iot/async/ConnectAwait.h>
#include <jde/iot/uatypes/UAClient.h>

#define var const auto

namespace Jde::Iot
{
	flat_map<string,vector<HCoroutine>> _requests; mutex _requestMutex;

	α ConnectAwait::await_ready()ι->bool{
		_result.Set( UAClient::Find(_id) );
		return _result.HasShared();
	}

	α ConnectAwait::await_suspend( HCoroutine h )ι->void{
		IAwait::await_suspend( h );
		if( auto pClient = UAClient::Find(_id); pClient )
			Jde::Resume( move(pClient), move(h) );
		else{
			_requestMutex.lock();
			if( auto p = _requests.find( _id ); p!=_requests.end() ){
				p->second.push_back( move(h) );
				_requestMutex.unlock();
			}
			else{
				_requests[_id] = {h};
				_requestMutex.unlock();
				Create( _id );
			}
		}
	}
	α ConnectAwait::Create( string opcServerId )ι->Task{
		try{
			auto pServer = ( co_await OpcServer::Select(opcServerId) ).UP<OpcServer>(); THROW_IF( !pServer, "Could not find opc server:  '{}'", opcServerId );
			auto pClient = ms<UAClient>( move(*pServer) );
			pClient->Connect();
		}
		catch( const IException& e ){
			lg _{ _requestMutex };
			var ua = dynamic_cast<const UAException*>( &e );
			for( auto& h : _requests[opcServerId] )
				Jde::Resume( ua ? UAException{*ua} : Exception{e.what(), e.Code, e.Level(), e.Stack().front()}, move(h) );
			_requests.erase( move(opcServerId) );
		}
	}
	α ConnectAwait::Resume( sp<UAClient> pClient, str target, function<void(HCoroutine&&)> resume )ι->void{
		ASSERT( pClient );
		vector<HCoroutine> handles;
		{
			lg _{ _requestMutex };
			handles = move( _requests[target] );
			_requests.erase( target );
		}
		for( auto h : handles )
			resume( move(h) );
	}

	α ConnectAwait::Resume( sp<UAClient>&& pClient, str target )ι->void{
		Resume( pClient, target, [p=pClient](HCoroutine&& h){ Jde::Resume((sp<UAClient>)move(p), move(h)); } );
	}
	α ConnectAwait::Resume( sp<UAClient>&& pClient, str target, const UAException&& e )ι->void{
		Resume( move(pClient), target, [sc=e.Code](HCoroutine&& h){ Jde::Resume(UAException{(StatusCode)sc}, move(h)); } );
	}
}