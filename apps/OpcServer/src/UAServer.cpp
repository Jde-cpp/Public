#include "UAServer.h"
#include <jde/fwk/exceptions/IOException.h>
#include <jde/fwk/process/thread.h>
#include <jde/opc/uatypes/opcHelpers.h>
#include <NodesetLoader/backendOpen62541.h>
#include "UATrust.h"


#define let const auto
namespace Jde::Opc::Server {
	UAServer::UAServer()ε:
		ServerName{ Settings::FindString("/opcServer/name").value_or("OpcServer") },
		_ua{ UA_Server_newWithConfig(&_config) }
	{
		if( _ua )
			UATrust::Install( *_ua );//must target the server-owned config - UA_Server_newWithConfig memset _config. Main thread, before Run() spawns the loop thread.
	}
	UAServer::~UAServer(){
		INFOT( ELogTags::App, "Stopping OPC UA server..." );
		if( _thread.has_value() ){
			_running = false;
#ifdef _MSC_VER
			std::this_thread::sleep_for( 1s );
#endif
			_thread->request_stop();
			_thread->join();
			_thread.reset();
		}
		if( _ua ){
			UA_Server_delete( _ua );
			_ua = nullptr;
		}
	}

	α UAServer::Run()ε->void{
		if( _thread )
			return;
		UAε( UA_Server_run_startup(_ua) );
		_running = true;//set before spawning: otherwise a destructor racing an early Run() could clear it before the thread reads it, and join() would hang.
		_thread = std::jthread{ [this](std::stop_token st){
			Thread::SetName( "UAServer" );
			//UA_Server_run(...) demands a volatile UA_Boolean*; run the loop manually against a std::atomic instead. This is what UA_Server_run does internally, and it also drops the original's double run_shutdown.
			while( _running && !st.stop_requested() )
				UA_Server_run_iterate( _ua, true );
			UA_Server_run_shutdown( _ua );//the delete is the destructor's, after it joins this thread.
			INFOT( ELogTags::App, "OPC UA server stopped." );
		}};
	}
	α UAServer::Load( fs::path configFile, SL sl )ε->void{
		INFOT( ELogTags::App, "Loading configuration from: '{}'", configFile.string() );
		CHECK_PATH( configFile, sl );
		//xmlSetGenericErrorFunc( nullptr, myXmlError );
		auto success = NodesetLoader_loadFile( _ua, configFile.string().c_str(), nullptr );
		THROW_IFSL( !success, "Failed to load nodeset file: '{}'", configFile.string() );
	}

	α UAServer::Namespaces()ι->flat_map<uint,string>{
		flat_map<uint,string> y;
		UA_String ns;
		for( uint i=0; !UA_Server_getNamespaceByIndex(_ua, i, &ns); ++i ){
			y.emplace( i, Opc::ToString(ns) );
			UA_String_clear( &ns );
		}
		return y;
	}
}
