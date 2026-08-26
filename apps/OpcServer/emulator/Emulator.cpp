#include "Emulator.h"
#include <jde/fwk/chrono.h>
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/str.h>
#include <jde/fwk/crypto/CryptoSettings.h>
#include <jde/fwk/crypto/OpenSsl.h>
#include "EmulatorClient.h"
#include "PlcServer.h"
#include "Signals.h"

#define let const auto
namespace Jde::Opc::Emulator{
	constexpr ELogTags _tags{ ELogTags::App };

	//-<arg>=<value> beats the settings file, as the soak driver's do.
	Ω argDuration( sv arg, sv path, Duration dflt )ε->Duration{
		if( auto a = Process::FindArg(string{arg}); a && a->size() )
			return Chrono::ToDuration( string{*a} );
		return Settings::FindDuration( path ).value_or( dflt );
	}
	Ω argString( sv arg, sv path, sv dflt )ι->string{
		if( auto a = Process::FindArg(string{arg}); a && a->size() )
			return *a;
		return Settings::FindString( path ).value_or( string{dflt} );
	}
	//the UA channel cert: /emulator/ssl puts it under the PlcEmulator product dir with the certificateUri as its SAN.
	Ω opcCertificate()ι->Crypto::CryptoSettings{ return Crypto::CryptoSettings{ Settings::FindDefaultObject("/emulator/ssl") }; }

	//pubsub: process values publish over Part 14 and anything the contract does not list is written over the session;
	//write: everything over the session - the PLC server is not started.  Commands are always subscribed.
	enum class ETransport : uint8{ PubSub, Write };
	struct Device; struct Emulator;
	struct Tag{
		TagSpec Spec;
		up<IGenerator> Generator;//null for command tags.
		optional<uint> Field;//index into the PubSub contract when published; else written over the client session.
		NodeId Node;//on the OpcServer - command tags and session-written tags, resolved per session.
		double Value{};
		bool Seen{};//a monitored item's first notification is the current value, not a change.
		Device* Owner{};
		Emulator* Self{};
	};
	struct Device{
		string Path, Name;//Name = the last path segment: "pumps~pump1" -> "pump1"; the contract's field names are "<Name>.<tag>".
		vector<Tag> Tags;
		bool Command{ true };//the last `status` seen - the run command the UI owns.
	};

	struct Emulator final : noncopyable{
		Emulator( sp<App::Client::IAppClient> app )ε;
		α Run()ε->int;
	private:
		α ParseDevices( SL sl )ε->void;
		α Connect()ε->void;//session, browse-path resolution, command subscriptions.
		α Reconnect()ε->void;
		α Cycle( Duration dt )ι->void;
		α LogStatus()ι->void;
		Ω OnDataChange( UA_Client*, UA_UInt32, void*, UA_UInt32, void* monContext, UA_DataValue* value )ι->void;

		sp<App::Client::IAppClient> _app;
		ETransport _transport;
		string _url, _certificateUri, _nsUri, _alias;
		Duration _period, _statusPeriod, _reconnectMin, _reconnectMax, _reconnectDelay;
		optional<Duration> _duration;
		up<PlcServer> _plc;
		up<EmulatorClient> _client;
		vector<Device> _devices;
		uint _cycles{}, _published{}, _writes{}, _writeFailures{}, _consecutiveFailures{}, _externalChanges{}, _reconnects{};
	};

	Emulator::Emulator( sp<App::Client::IAppClient> app )ε:
		_app{ move(app) },
		_transport{ ETransport::PubSub },
		_url{ argString("-url", "/emulator/url", "opc.tcp://127.0.0.1:4840") },
		_certificateUri{ Settings::FindString("/emulator/certificateUri").value_or("urn:open62541.server.application") },
		_period{ argDuration("-period", "/emulator/period", 1s) },
		_statusPeriod{ argDuration("-statusPeriod", "/emulator/statusPeriod", 1min) },
		_reconnectMin{ argDuration("-reconnectMin", "/emulator/reconnectMin", 1s) },
		_reconnectMax{ argDuration("-reconnectMax", "/emulator/reconnectMax", 30s) },
		_reconnectDelay{ _reconnectMin }{
		let transport = argString( "-transport", "/emulator/transport", "pubsub" );
		THROW_IF( transport!="pubsub" && transport!="write", "transport '{}' must be pubsub or write.", transport );
		_transport = transport=="write" ? ETransport::Write : ETransport::PubSub;
		if( let d = argDuration("-duration", "/emulator/duration", Duration::zero()); d>Duration::zero() )
			_duration = d;
		PubSub::Config contract{ Settings::AsObject("/emulator/pubsub") };
		_nsUri = contract.Namespace;
		_alias = contract.DataSetName;
		if( _transport==ETransport::PubSub ){
			let nodeset = Settings::FindPath( "/emulator/plc/nodeset" ); THROW_IF( !nodeset, "/emulator/plc/nodeset is required - the nodeset the OpcServer loads." );
			_plc = mu<PlcServer>( Settings::FindNumber<UA_UInt16>("/emulator/plc/port").value_or(4841), *nodeset, move(contract) );
		}
		ParseDevices( SRCE_CUR );
		_client = mu<EmulatorClient>( _url, _certificateUri, opcCertificate(), Ƒ("{:x}", _app->SessionId()) );
	}

	α Emulator::ParseDevices( SL sl )ε->void{
		for( let& d : Settings::FindDefaultArray("/emulator/devices") ){
			let& o = Json::AsObject( d, sl );
			Device device{ Json::AsString(o, "path", sl) };
			let lastSegment = device.Path.substr( device.Path.rfind('/')+1 );
			device.Name = lastSegment.substr( lastSegment.rfind('~')+1 );
			for( let& t : Json::AsArray(o, "tags", sl) ){
				Tag tag{ TagSpec{Json::AsObject(t, sl), sl} };
				if( tag.Spec.Mode!=EMode::Command ){
					tag.Generator = MakeGenerator( tag.Spec );
					if( _plc )
						tag.Field = _plc->FindField( Ƒ("{}.{}", device.Name, tag.Spec.Name) );
				}
				device.Tags.push_back( move(tag) );
			}
			THROW_IFSL( device.Tags.empty(), "Device '{}' has no tags.", device.Path );
			_devices.push_back( move(device) );
		}
		THROW_IFSL( _devices.empty(), "/emulator/devices is empty." );
		for( auto& device : _devices ){//after every push_back: the vectors do not move again.
			vector<string> routes;
			for( auto& tag : device.Tags ){
				tag.Owner = &device;
				tag.Self = this;
				routes.push_back( Ƒ("{}={}{}", tag.Spec.Name, ToString(tag.Spec.Mode), tag.Spec.Mode==EMode::Command ? " (subscribed)" : tag.Field ? " (published)" : " (written)") );
			}
			INFO( "[{}]{}", device.Name, Str::Join(routes, ", ") );
		}
	}

	α Emulator::Connect()ε->void{
		_client->Connect();
		flat_map<string,NsIndex> aliases;
		if( _nsUri.size() )
			aliases.emplace( _alias, _client->Namespace(_nsUri) );
		UA_UInt32 subscription{};
		uint commands{};
		for( auto& device : _devices ){
			for( auto& tag : device.Tags ){
				let command = tag.Spec.Mode==EMode::Command;
				if( !command && tag.Field )
					continue;//published - the OpcServer's reader owns that node's writes.
				tag.Node = _client->Resolve( Ƒ("{}/{}~{}", device.Path, _alias, tag.Spec.Name), 0, aliases );
				tag.Seen = false;
				if( command ){
					if( !subscription )
						subscription = _client->CreateSubscription( _period );
					_client->Monitor( subscription, tag.Node, _period, &tag, OnDataChange );
					++commands;
				}
				DBG( "[{}]{} -> {} ({})", device.Name, tag.Spec.Name, tag.Node.ToString(), command ? "subscribed" : "written" );
			}
		}
		_reconnectDelay = _reconnectMin;
		INFO( "Connected to '{}': {} command tag(s) subscribed.", _url, commands );
	}

	α Emulator::OnDataChange( UA_Client*, UA_UInt32, void*, UA_UInt32, void* monContext, UA_DataValue* value )ι->void{
		auto tag = (Tag*)monContext;
		if( !tag || !value || !value->hasValue || !UA_Variant_hasScalarType(&value->value, &UA_TYPES[UA_TYPES_BOOLEAN]) )
			return;
		let command = *(UA_Boolean*)value->value.data;
		auto& device = *tag->Owner;
		if( !tag->Seen ){//the initial notification carries the current value.
			tag->Seen = true;
			device.Command = command;
			DBG( "[{}]{} = {} (initial)", device.Name, tag->Spec.Name, command );
		}
		else if( device.Command!=command ){
			device.Command = command;
			++tag->Self->_externalChanges;
			INFO( "[{}]{} <- {} (external)", device.Name, tag->Spec.Name, command );
		}
	}

	α Emulator::Cycle( Duration dt )ι->void{
		for( auto& device : _devices ){
			for( auto& tag : device.Tags ){
				if( !tag.Generator )
					continue;
				tag.Value = tag.Generator->Next( dt, device.Command );
				if( tag.Field ){
					try{
						_plc->Write( *tag.Field, tag.Value );
						++_published;
					}
					catch( const std::exception& e ){
						WARN( "[{}]local write {} failed: {}", device.Name, tag.Spec.Name, e.what() );
					}
					continue;
				}
				UA_Variant v;
				UA_Boolean b = tag.Value!=0;
				if( tag.Spec.IsBool() )
					UA_Variant_setScalar( &v, &b, &UA_TYPES[UA_TYPES_BOOLEAN] );
				else
					UA_Variant_setScalar( &v, &tag.Value, &UA_TYPES[UA_TYPES_DOUBLE] );
				try{
					_client->Write( tag.Node, v );
					++_writes;
					_consecutiveFailures = 0;
				}
				catch( const std::exception& e ){
					++_writeFailures;
					++_consecutiveFailures;
					WARN( "[{}]write {} failed: {}", device.Name, tag.Spec.Name, e.what() );
				}
			}
		}
		++_cycles;
	}

	α Emulator::Reconnect()ε->void{
		++_reconnects;
		_consecutiveFailures = 0;
		WARN( "Session to '{}' lost - reconnecting (#{}) in {}.", _url, _reconnects, Chrono::ToString(_reconnectDelay) );
		_client->Disconnect();
		for( let until = steady_clock::now()+_reconnectDelay; steady_clock::now()<until && !Process::ShuttingDown(); std::this_thread::sleep_for(100ms) ){
			if( _plc )
				_plc->Iterate();//keep the PLC's own server and publisher alive while the session is down.
		}
		if( Process::ShuttingDown() )
			return;
		_reconnectDelay = std::min( _reconnectDelay*2, _reconnectMax );
		try{
			Connect();
		}
		catch( const std::exception& e ){
			WARN( "Reconnect failed: {}", e.what() );
		}
	}

	α Emulator::LogStatus()ι->void{
		vector<string> values;
		for( let& device : _devices ){
			vector<string> tags;
			for( let& tag : device.Tags )
				tags.push_back( tag.Generator ? Ƒ("{}={:.1f}", tag.Spec.Name, tag.Value) : Ƒ("{}={}", tag.Spec.Name, device.Command) );
			values.push_back( Ƒ("{}[{}]", device.Name, Str::Join(tags, " ")) );
		}
		let summary = Ƒ( "cycles={} published={} writes={} writeFailures={} externalChanges={} reconnects={} - {}", _cycles, _published, _writes, _writeFailures, _externalChanges, _reconnects, Str::Join(values, " ") );
		INFO( "{}", summary );//pre-formatted: a `{:.1f}` inside the log message itself is a MemoryLog::Find self-deadlock.
	}

	α Emulator::Run()ε->int{
		try{
			Connect();
		}
		catch( const std::exception& e ){
			WARN( "Initial connect to '{}' failed: {} - retrying.", _url, e.what() );
		}
		let start = steady_clock::now();
		auto lastCycle = start;
		auto nextCycle = start+_period;
		auto nextStatus = start+_statusPeriod;
		optional<steady_clock::time_point> deadline;
		if( _duration )
			deadline = start+*_duration;
		INFO( "Emulator running: transport={}, period={}, devices={}{}.", _transport==ETransport::PubSub ? "pubsub" : "write", Chrono::ToString(_period), _devices.size(), _duration ? Ƒ(", duration={}", Chrono::ToString(*_duration)) : string{} );
		while( !Process::ShuttingDown() && (!deadline || steady_clock::now()<*deadline) ){
			if( _plc )
				_plc->Iterate();//the publisher's timer fires in here.
			if( !_client->IsActivated() || _consecutiveFailures>=3 ){
				Reconnect();
				continue;
			}
			let slice = std::clamp( duration_cast<milliseconds>(nextCycle-steady_clock::now()), 0ms, 50ms );
			_client->Iterate( (uint32)slice.count() );//command notifications arrive in here.
			if( let now = steady_clock::now(); now>=nextCycle ){
				Cycle( duration_cast<Duration>(now-lastCycle) );
				lastCycle = now;
				nextCycle += _period;
				if( nextCycle<now )
					nextCycle = now+_period;
			}
			if( steady_clock::now()>=nextStatus ){
				LogStatus();
				nextStatus += _statusPeriod;
			}
		}
		LogStatus();
		_client->Disconnect();
		return EXIT_SUCCESS;
	}

	α Run( sp<App::Client::IAppClient> client )ε->int{
		Emulator emulator{ move(client) };
		return emulator.Run();
	}

	α GrantWriteRights( const sp<App::Client::IAppClient>& client )ι->void{
		try{
			let schema = argString( "-opcSchema", "/emulator/opcSchema", _debug ? "opc.debug" : "opc.release" );
			constexpr uint allAccess{ 0x7F };//the UA access-level byte: read|write|historyRead|historyWrite|semanticChange|statusWrite|timestampWrite.
			client->QuerySync<jvalue>(
				"createAcl( identity:{id:$userId}, permissionRight:{ allowed:$allowed, denied:0, resource:{schemaName:$schemaName, target:\"nodeIds\"}} )",
				{{"userId", client->UserPK().Value}, {"allowed", allAccess}, {"schemaName", schema}} );
			INFO( "Granted OPC node access for user {} on '{}' - restart the OpcServer to load it.", client->UserPK().Value, schema );
		}
		catch( const std::exception& e ){
			INFO( "createAcl failed (already granted on a previous run?): {}", e.what() );
		}
	}

	α CreateCertificates()ε->void{
		Crypto::CryptoSettings http{ Json::FindDefaultObject(Settings::AsObject("/http"), "ssl"), Process::ProductName() };
		Crypto::EnsureKeyCertificate( http );
		INFO( "AppServer login certificate: {}", http.Certificate.Path.string() );
		let opc = opcCertificate();
		Crypto::EnsureKeyCertificate( opc );
		INFO( "OPC UA client certificate: {} (SAN '{}') - its directory must be in the OpcServer's /access/trustedCertDirs.", opc.Certificate.Path.string(), opc.Certificate.SubjectAltName );
	}
}
