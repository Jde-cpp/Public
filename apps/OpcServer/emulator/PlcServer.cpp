#include "PlcServer.h"
#include <NodesetLoader/backendOpen62541.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/fwk/exceptions/IOException.h>

#define let const auto
namespace Jde::Opc::Emulator{
	constexpr ELogTags _tags{ (ELogTags)EOpcLogTags::PubSub };
	static Opc::Logger _logger{};

	PlcServer::PlcServer( UA_UInt16 port, const fs::path& nodeset, PubSub::Config&& contract, SL sl )ε:
		_port{ port },
		_contract{ move(contract) }{
		UA_ServerConfig config{};
		config.logging = &_logger;
		let sc = UA_ServerConfig_setMinimal( &config, port, nullptr ); THROW_IFX( sc, UAException(sc, "UA_ServerConfig_setMinimal", {}, sl) );
		_server = UA_Server_newWithConfig( &config );
		THROW_IFSL( !_server, "UA_Server_newWithConfig failed." );
		try{
			CHECK_PATH( nodeset, sl );
			THROW_IFSL( !NodesetLoader_loadFile(_server, nodeset.string().c_str(), nullptr), "Could not load nodeset '{}'.", nodeset.string() );
			_contract.Resolve( *_server, sl );
			_writer = mu<PubSub::Writer>( *_server, _contract, sl );
			let startup = UA_Server_run_startup( _server ); THROW_IFX( startup, UAException(startup, "UA_Server_run_startup", {}, sl) );
		}
		catch( ... ){
			UA_Server_delete( _server );
			_server = nullptr;
			throw;
		}
		INFO( "PLC server up on port {} with '{}'; publishing {}.", port, nodeset.filename().string(), _contract.ToString() );
	}
	PlcServer::~PlcServer(){
		if( _server ){
			UA_Server_run_shutdown( _server );
			UA_Server_delete( _server );
		}
	}
	α PlcServer::Iterate()ι->void{
		UA_Server_run_iterate( _server, false );
	}
	α PlcServer::FindField( sv name )Ι->optional<uint>{
		for( uint i=0; i<_contract.Fields.size(); ++i ){
			if( _contract.Fields[i].Name==name )
				return i;
		}
		return {};
	}
	α PlcServer::Write( uint field, double value )ε->void{
		let& f = _contract.Fields.at( field );
		UA_Variant v;
		if( f.Type==&UA_TYPES[UA_TYPES_BOOLEAN] ){
			UA_Boolean b = value!=0;
			UA_Variant_setScalar( &v, &b, f.Type );
			UAε( UA_Server_writeValue(_server, f.Node, v) );
		}
		else if( f.Type==&UA_TYPES[UA_TYPES_FLOAT] ){
			UA_Float x = (UA_Float)value;
			UA_Variant_setScalar( &v, &x, f.Type );
			UAε( UA_Server_writeValue(_server, f.Node, v) );
		}
		else{
			THROW_IF( f.Type!=&UA_TYPES[UA_TYPES_DOUBLE], "Field '{}' is {} - the emulator writes Boolean, Float or Double.", f.Name, f.Type->typeName );
			UA_Variant_setScalar( &v, &value, f.Type );
			UAε( UA_Server_writeValue(_server, f.Node, v) );
		}
	}
}
