#include <jde/opc/async/ConnectAwait.h>
#include <jde/opc/uatypes/UAClient.h>

#define let const auto

namespace Jde::Opc
{
	flat_map<tuple<string,string>,vector<ConnectAwait::Handle>> _requests; mutex _requestMutex;

	α ConnectAwait::Suspend()ι->void{
		if( auto pClient = UAClient::Find(_opcTarget, _loginName, _password); pClient )
			base::Resume( move(pClient) );
		else{
			auto key = make_tuple( _opcTarget,_loginName );
			_requestMutex.lock();
			if( auto p = _requests.find(key); p!=_requests.end() ){
				p->second.push_back( _h );
				_requestMutex.unlock();
			}
			else{
				_requests[key] = {_h};
				_requestMutex.unlock();
				Create();
			}
		}
	}
	α ConnectAwait::Create()ι->OpcServerAwait::Task{
		try{
			auto servers = co_await OpcServerAwait{ _opcTarget };
			THROW_IF( servers.empty(), "Could not find opc server:  '{}'", _opcTarget );
			auto pClient = ms<UAClient>( move(servers.front()), _loginName, _password );
			pClient->Connect();
		}
		catch( const IException& e ){
			lg _{ _requestMutex };
			let ua = dynamic_cast<const UAException*>( &e );
			auto key = make_tuple( move(_opcTarget), move(_loginName) );
			for( auto& h : _requests[key] ){
				if( ua )
					h.promise().ResumeExp( UAException{*ua}, h );
				else
					h.promise().ResumeExp( Exception{e.what(), e.Code, e.Level(), e.Stack().front()}, h );
			}
			_requests.erase( key );
		}
	}
	α ConnectAwait::Resume( sp<UAClient> pClient, str target, str loginName, function<void(ConnectAwait::Handle)> resume )ι->void{
		ASSERT( pClient );
		vector<ConnectAwait::Handle> handles;
		{
			lg _{ _requestMutex };
			auto key = make_tuple( target, loginName );
			if( auto p = _requests.find( key ); p!=_requests.end() ){
				handles = _requests[key];
				_requests.erase( key );
			}
		}
		for( auto h : handles )
			resume( h );
	}

	α ConnectAwait::Resume( sp<UAClient> client, str target, str loginName )ι->void{
		Resume( client, target, loginName, [=](ConnectAwait::Handle h)mutable{ h.promise().Resume(move(client), h); } );
	}
	α ConnectAwait::Resume( sp<UAClient> client, str target, str loginName, const UAException&& e )ι->void{
		Resume( move(client), target, loginName, [sc=e.Code](ConnectAwait::Handle h){ h.promise().ResumeExp(UAException{(StatusCode)sc}, h); } );
	}
}