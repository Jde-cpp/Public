#include "SoakRunner.h"
#include <algorithm>
#include <condition_variable>
#include <fstream>
#include <jde/fwk/process/execution.h>
#include <jde/fwk/settings.h>
#include <jde/opc/uatypes/ExNodeId.h>
#include <jde/opc/uatypes/Value.h>
#include "../tests/utils/GatewayClientSocket.h"
#include "../src/types/proto/opc.FromServer.h"

#define let const auto

namespace Jde::Opc::Gateway::Soak{
	constexpr ELogTags _tags{ ELogTags::Test };
	using Tests::GatewayClientSocket;
	using Web::Client::ClientSocketAwait;

	//-<arg>=<value> beats the settings file so soak.sh can vary a run without another jsonnet.
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

	struct SoakRunner final : Tests::IListener, std::enable_shared_from_this<SoakRunner>{
		SoakRunner( sp<App::Client::IAppClient> client )ε;
		α Run()ε->int;
		α OnData( string opcId, NodeId nodeId, const vector<FromServer::Value>& values )ι->void override;
	private:
		α Connect()ε->void;
		α EnsureServerConnection()ε->void;
		α Subscribe()ε->void;
		α WriteCycle()ε->void;//throws only when a reconnect attempt fails - anything else is counted and survived.
		α SampleStatus()ι->void;
		α Reconnect()ε->void;
		α Percentile( double p )Ι->uint;
		α WriteSummary( sv verdict )ι->void;

		sp<App::Client::IAppClient> _client;
		sp<GatewayClientSocket> _socket;
		vector<sp<GatewayClientSocket>> _retiredSockets;//dropped sockets stay alive - Connect() registered them with Process::AddShutdown.
		ServerCnnctnNK _target;
		vector<NodeId> _nodes;
		string _urn, _url, _host;
		PortType _port;
		Duration _duration, _writePeriod, _pushTimeout, _statusPeriod, _quietInterval, _quietPeriod;
		fs::path _csvPath, _summaryPath;
		std::ofstream _csv;

		std::mutex _mutex; std::condition_variable _cv;
		flat_map<NodeId,uint> _latest;

		uint _counter{}, _writeIndex{}, _consecutiveFailures{};
		uint _writes{}, _writeFailures{}, _pushes{}, _misses{}, _socketDrops{}, _statusFailures{}, _quietWindows{};
		vector<uint32> _latenciesMs;
		bool _completed{};
	};

	SoakRunner::SoakRunner( sp<App::Client::IAppClient> client )ε:
		_client{ move(client) },
		_target{ argString("-target", "/soak/target", "OpcSoak") },
		_urn{ Settings::FindString("/opc/urn").value_or("urn:open62541.server.application") },
		_url{ Settings::FindString("/opc/url").value_or("opc.tcp://127.0.0.1:4840") },
		_host{ argString("-gatewayHost", "/soak/gatewayHost", "localhost") },
		_port{ Settings::FindNumber<PortType>("/soak/gatewayPort").value_or(1968) },
		_duration{ argDuration("-duration", "/soak/duration", std::chrono::hours{24}) },
		_writePeriod{ argDuration("-writePeriod", "/soak/writePeriod", 1s) },
		_pushTimeout{ argDuration("-pushTimeout", "/soak/pushTimeout", 5s) },
		_statusPeriod{ argDuration("-statusPeriod", "/soak/statusPeriod", 1min) },
		_quietInterval{ argDuration("-quietInterval", "/soak/quietInterval", std::chrono::hours{6}) },
		_quietPeriod{ argDuration("-quietPeriod", "/soak/quietPeriod", 10min) },
		_csvPath{ argString("-csv", "/soak/csv", "soak.csv") },
		_summaryPath{ argString("-summary", "/soak/summary", "summary.json") }
	{
		if( let nodes = Json::FindArray(Settings::Value(), "/soak/nodes"); nodes ){
			for( let& n : *nodes ){
				let& o = n.as_object();
				_nodes.push_back( NodeId{ o.at("ns").to_number<uint16_t>(), o.at("id").to_number<uint32_t>() } );
			}
		}
		if( _nodes.empty() )
			_nodes.push_back( NodeId{4, 6017} );
		_latenciesMs.reserve( 1'000 );
	}

	α GrantWriteRights( const sp<App::Client::IAppClient>& client )ι->void{
		try{
			let schema = argString( "-opcSchema", "/soak/opcSchema", _debug ? "opc.debug" : "opc.release" );
			constexpr uint allAccess{ 0x7F };//the UA access-level byte: read|write|historyRead|historyWrite|semanticChange|statusWrite|timestampWrite.
			client->QuerySync<jvalue>(
				"createAcl( identity:{id:$userId}, permissionRight:{ allowed:$allowed, denied:0, resource:{schemaName:$schemaName, target:\"nodeIds\"}} )",
				{{"userId", client->UserPK().Value}, {"allowed", allAccess}, {"schemaName", schema}} );
			INFO( "Granted OPC node access for user {} on '{}'.", client->UserPK().Value, schema );
		}
		catch( const std::exception& e ){
			INFO( "createAcl failed (already granted on a previous run?): {}", e.what() );
		}
	}

	α SoakRunner::Connect()ε->void{
		optional<ssl::context> ctx;
		_socket = ms<GatewayClientSocket>( Executor(), ctx );
		BlockVoidAwait( _socket->RunSession(_host, _port) );
		BlockAwait<ClientSocketAwait<uint32>,uint>( _socket->Connect(_client->SessionId()) );
		INFO( "Connected to gateway {}:{}.", _host, _port );
	}

	α SoakRunner::EnsureServerConnection()ε->void{
		let existing = _socket->QuerySync( Ƒ("serverConnection( target: \"{}\" ){{ id url }}", _target) );
		if( existing.is_object() && existing.get_object().contains("id") ){
			INFO( "Server connection '{}' exists: {}.", _target, serialize(existing) );
			return;
		}
		let created = _socket->QuerySync( Ƒ("mutation createServerConnection( target:\"{}\", name:\"Soak test server\", certificateUri:\"{}\", description:\"Soak test connection\", url:\"{}\", isDefault:false ){{id}}", _target, _urn, _url) );
		INFO( "Created server connection '{}': {}.", _target, serialize(created) );
	}

	α SoakRunner::Subscribe()ε->void{
		let ack = BlockAwait<ClientSocketAwait<FromServer::SubscriptionAck>,FromServer::SubscriptionAck>( _socket->Subscribe(_target, _nodes, shared_from_this()) );
		THROW_IF( (uint)ack.results_size()!=_nodes.size(), "Subscription ack has {} results for {} nodes.", ack.results_size(), _nodes.size() );
		for( int i=0; i<ack.results_size(); ++i )
			THROW_IF( ack.results(i).status_code(), "Subscription for node '{}' failed: {:x}.", _nodes[i].ToString(), ack.results(i).status_code() );
		INFO( "Subscribed to {} node(s) on '{}'.", _nodes.size(), _target );
	}

	α SoakRunner::OnData( string opcId, NodeId nodeId, const vector<FromServer::Value>& values )ι->void{
		if( values.empty() )
			return;
		try{
			let v = FromServer::ToValue( values.back() ).AsNumber<uint>();
			{
				std::lock_guard _{ _mutex };
				_latest[nodeId] = v;
			}
			TRACE( "OnData: {} {}={}.", opcId, nodeId.ToString(), v );
			_cv.notify_all();
		}
		catch( const std::exception& e ){
			WARN( "OnData: could not convert value for {}: {}", nodeId.ToString(), e.what() );
		}
	}

	α SoakRunner::Reconnect()ε->void{
		++_socketDrops;
		_consecutiveFailures = 0;
		WARN( "Reconnecting to the gateway (drop #{}).", _socketDrops );
		if( _socket )
			_retiredSockets.push_back( move(_socket) );
		Connect();
		Subscribe();
	}

	α SoakRunner::WriteCycle()ε->void{
		let& node = _nodes[_writeIndex++ % _nodes.size()];
		let value = ++_counter;
		let start = steady_clock::now();
		try{
			//no {value} result-request: the subscription push is the round-trip assertion, and the mutation's read-back
			//never resumes when UA responses land in the same run_iterate (split-process localhost; see soak findings).
			string q{ "updateVariable( opc: $opc, id: $id, value: $value )" };
			_socket->QuerySync( move(q), jobject{ {"opc",_target}, {"id",node.ToJson()}, {"value",value} } );
			++_writes;
			_consecutiveFailures = 0;
		}
		catch( const std::exception& e ){
			++_writeFailures;
			WARN( "updateVariable failed for {}: {}", node.ToString(), e.what() );
			if( ++_consecutiveFailures>=3 )
				Reconnect();
			return;
		}
		std::unique_lock lock{ _mutex };
		if( _cv.wait_for(lock, _pushTimeout, [&]{ auto p = _latest.find(node); return p!=_latest.end() && p->second==value; }) ){
			++_pushes;
			_latenciesMs.push_back( (uint32)duration_cast<milliseconds>(steady_clock::now()-start).count() );
		}
		else{
			++_misses;
			WARN( "No data-change push for {} value {} within {}.", node.ToString(), value, Chrono::ToString(_pushTimeout) );
		}
	}

	α SoakRunner::Percentile( double p )Ι->uint{
		if( _latenciesMs.empty() )
			return 0;
		auto v = _latenciesMs;
		let i = std::min( v.size()-1, (size_t)(p*v.size()) );
		std::nth_element( v.begin(), v.begin()+i, v.end() );
		return v[i];
	}

	α SoakRunner::SampleStatus()ι->void{
		try{
			string q{ "status{ memory startTime uptimeSeconds clients monitoredItems }" };
			let j = _socket->QuerySync( move(q) );
			let& o = j.as_object();
			let num = [&o]( sv name )->int64_t{ auto p = o.find(name); return p==o.end() ? -1 : p->value().to_number<int64_t>(); };
			_csv << ToIsoString( Clock::now() )
				<< ',' << num("memory") << ',' << num("uptimeSeconds") << ',' << num("clients") << ',' << num("monitoredItems")
				<< ',' << _writes << ',' << _pushes << ',' << _misses << ',' << _writeFailures << ',' << _socketDrops << ',' << _statusFailures
				<< ',' << Percentile(.5) << ',' << Percentile(.99) << std::endl;
		}
		catch( const std::exception& e ){
			++_statusFailures;
			WARN( "Status query failed: {}", e.what() );
		}
	}

	α SoakRunner::WriteSummary( sv verdict )ι->void{
		try{
			jobject y{
				{"verdict", verdict}, {"completed", _completed}, {"writes", _writes}, {"pushes", _pushes}, {"misses", _misses},
				{"writeFailures", _writeFailures}, {"socketDrops", _socketDrops}, {"statusFailures", _statusFailures},
				{"quietWindows", _quietWindows}, {"p50Ms", Percentile(.5)}, {"p99Ms", Percentile(.99)},
				{"maxMs", _latenciesMs.empty() ? 0 : *std::ranges::max_element(_latenciesMs)}
			};
			std::ofstream f{ _summaryPath };
			f << serialize( y );
			INFO( "Summary written to '{}'.", _summaryPath.string() );
		}
		catch( const std::exception& e ){
			WARN( "Could not write summary: {}", e.what() );
		}
	}

	α SoakRunner::Run()ε->int{
		_csv.open( _csvPath, std::ios::app );
		THROW_IF( !_csv.is_open(), "Could not open csv '{}'.", _csvPath.string() );
		if( _csv.tellp()==0 )
			_csv << "time,memory,uptimeSeconds,clients,monitoredItems,writes,pushes,misses,writeFailures,socketDrops,statusFailures,p50Ms,p99Ms" << std::endl;
		Connect();
		EnsureServerConnection();
		Subscribe();
		if( let d = argDuration("-startDelay", "/soak/startDelay", 0s); d>Duration::zero() )
			std::this_thread::sleep_for( d );//see if a settle delay after the subscription ack avoids the first-write hang.
		let start = Clock::now();
		let deadline = start+_duration;
		auto nextStatus = start;
		auto nextQuiet = start+_quietInterval;
		INFO( "Soak started: duration={}, writePeriod={}, nodes={}, deadline={}.", Chrono::ToString(_duration), Chrono::ToString(_writePeriod), _nodes.size(), ToIsoString(deadline) );
		while( Clock::now()<deadline && !Process::ShuttingDown() ){
			let cycleStart = Clock::now();
			if( cycleStart>=nextStatus ){
				SampleStatus();
				nextStatus = cycleStart+_statusPeriod;
			}
			if( cycleStart>=nextQuiet ){
				++_quietWindows;
				let quietEnd = std::min( cycleStart+_quietPeriod, deadline );
				INFO( "Quiet window #{} until {} - subscription stays open, no writes.", _quietWindows, ToIsoString(quietEnd) );
				while( Clock::now()<quietEnd && !Process::ShuttingDown() ){
					if( Clock::now()>=nextStatus ){
						SampleStatus();
						nextStatus = Clock::now()+_statusPeriod;
					}
					std::this_thread::sleep_for( 1s );
				}
				nextQuiet = Clock::now()+_quietInterval;
				INFO( "Quiet window #{} over - resuming writes.", _quietWindows );
				continue;
			}
			WriteCycle();
			std::this_thread::sleep_until( cycleStart+_writePeriod );
		}
		_completed = Clock::now()>=deadline;
		try{
			BlockAwait<ClientSocketAwait<FromServer::UnsubscribeAck>,FromServer::UnsubscribeAck>( _socket->Unsubscribe(_target, _nodes) );
		}
		catch( const std::exception& e ){
			WARN( "Unsubscribe failed: {}", e.what() );
		}
		SampleStatus();
		let pass = _completed && !_misses && !_writeFailures && !_socketDrops && !_statusFailures;
		WriteSummary( pass ? "PASS" : "FAIL" );
		INFO( "Soak {}: completed={}, writes={}, pushes={}, misses={}, writeFailures={}, socketDrops={}, statusFailures={}, p50={}ms, p99={}ms.",
			pass ? "PASS" : "FAIL", _completed, _writes, _pushes, _misses, _writeFailures, _socketDrops, _statusFailures, Percentile(.5), Percentile(.99) );
		return pass ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	α Run( sp<App::Client::IAppClient> client )ε->int{
		auto runner = ms<SoakRunner>( move(client) );
		return runner->Run();
	}
}
