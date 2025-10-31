#include <jde/app/shared/proto/App.FromServer.h>
#include <jde/fwk/io/proto.h>
#include <jde/db/Row.h>
#include <jde/db/meta/Table.h>
#include <jde/db/meta/Column.h>
#include <jde/ql/types/TableQL.h>
#include <jde/web/Jwt.h>

#define let const auto

namespace Jde::App::FromServer{
	Ω setMessage( RequestId requestId, function<void(Proto::FromServer::Message&)> set )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		m.set_request_id( requestId );
		set( m );
		return t;
	}
}
namespace Jde::App{
	α FromServer::Ack( uint32 serverSocketId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		t.add_messages()->set_ack( serverSocketId );
		return t;
	}

	α FromServer::Complete( RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		t.add_messages()->set_request_id( requestId );
		return t;
	}
	α FromServer::ConnectionInfo( AppPK appPK, AppInstancePK instancePK, RequestId clientRequestId, const Crypto::PublicKey& appServerPubKey )ι->Proto::FromServer::Transmission{
		return setMessage( clientRequestId, [&](auto& m){
			auto& connectionInfo = *m.mutable_connection_info();
			connectionInfo.set_app_pk( appPK );
			connectionInfo.set_instance_pk( instancePK );
			connectionInfo.set_certificate_modulus( {appServerPubKey.Modulus.begin(), appServerPubKey.Modulus.end()} );
			connectionInfo.set_certificate_exponent( {appServerPubKey.Exponent.begin(), appServerPubKey.Exponent.end()} );
		});
	}

	α FromServer::Exception( const exception& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto& m = *t.add_messages();
		if( requestId )
			m.set_request_id( *requestId );
		auto& proto = *m.mutable_exception();
		proto.set_what( e.what() );
		if( let p = dynamic_cast<const IException*>(&e); p )
			proto.set_code( p->Code );
		return t;
	}
	α FromServer::Exception( string&& e, optional<RequestId> requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId.value_or(0), [&](auto& m){
			auto& proto = *m.mutable_exception();
			proto.set_what( move(e) );
		});
	}
	α FromServer::Jwt( Web::Jwt&& jwt, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			m.set_jwt( jwt.Payload() );
		});
	}
	α FromServer::ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status{
		Proto::FromServer::Status output;
		output.set_application_id( (google::protobuf::uint32)appId );
		output.set_instance_id( (google::protobuf::uint32)instanceId );
		output.set_host_name( hostName );
		*output.mutable_start_time() = input.start_time();
		output.set_memory( input.memory() );
		*output.mutable_values() = move( *input.mutable_details() );
		return output;
	}

	α FromServer::StatusBroadcast( Proto::FromServer::Status status )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_status() = move( status );
		return t;
	}
	α FromServer::SubscriptionAck( vector<QL::SubscriptionId>&& subscriptionIds, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			auto& ack = *m.mutable_subscription_ack();
			for_each( subscriptionIds, [&](auto id){ ack.add_server_ids(id); } );
		});
	}
	α FromServer::Subscription( string&& s, RequestId requestId )ι->Proto::FromServer::Transmission{
		return setMessage( requestId, [&](auto& m){
			*m.mutable_subscription() = move(s);
		});
	}

	α FromServer::ExecuteRequest( RequestId serverRequestId, UserPK userPK, string&& fromClient )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( serverRequestId );
		if( userPK ){
			auto customExecute = toServer->mutable_execute();
			customExecute->set_user_pk( userPK );
			*customExecute->mutable_transmission() = move( fromClient );
		}
		else
			toServer->set_execute_anonymous( move(fromClient) );
		return t;
	}

	α FromServer::Execute( string&& executionResult, RequestId clientRequestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( clientRequestId );
		toServer->set_execute_response( move(executionResult) );
		return t;
	}

	α FromServer::GraphQL( string&& queryResults, RequestId requestId )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( requestId );
		toServer->set_graph_ql( move(queryResults) );
		return t;
	}
	α FromServer::ToTrace( DB::Row&& row, const vector<QL::ColumnQL>& columns )ι->Proto::FromServer::Trace{
		Proto::FromServer::Trace t;
		uint i=0;
		for( auto&& c : columns ){
			if( !c.DBColumn )
				continue;

			str name = c.DBColumn->Name;
			if( name=="id" )
				t.set_id( row.GetUInt32(i) );
			else if( name=="instance_id" )
				t.set_instance_id( row.GetUInt32(i) );
			else if( name=="file_id" )
				t.set_file_id( Jde::Proto::ToBytes(row.GetGuid(i)) );
			else if( name=="function_id" )
				t.set_function_id( Jde::Proto::ToBytes(row.GetGuid(i)) );
			else if( name=="line_number" )
				t.set_line( row.GetUInt32(i) );
			else if( name=="message_id" )
				t.set_message_id( Jde::Proto::ToBytes(row.GetGuid(i)) );
			else if( name=="level" )
				t.set_level( (Log::Proto::ELogLevel)row.GetUInt16(i) );
			else if( name=="thread_id" )
				t.set_thread_id( row.GetUInt32(i) );
			else if( name=="time" )
				*t.mutable_time() = Jde::Proto::ToTimestamp( row.GetTimePoint(i) );
			else if( name=="user_pk" )
				t.set_user_pk( row.GetUInt32(i) );
			else
				BREAK;
			++i;
		}
		return t;
	}
	α FromServer::TraceBroadcast( LogPK id, AppPK appId, AppInstancePK instanceId, const Logging::Entry& m, const vector<string>& args )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto traces = t.add_messages()->mutable_traces();
		traces->set_app_id( appId );
		auto proto = traces->add_values();
		proto->set_id( id );
		proto->set_instance_id( instanceId );
		*proto->mutable_time() = Jde::Proto::ToTimestamp( m.Time );
		proto->set_level( (Log::Proto::ELogLevel)m.Level );
		proto->set_message_id( Jde::Proto::ToBytes(m.Id()) );
		proto->set_file_id( Jde::Proto::ToBytes(m.FileId()) );
		proto->set_function_id( Jde::Proto::ToBytes(m.FunctionId()) );
		proto->set_line( m.Line );
		proto->set_user_pk( m.UserPK.Value );
		for( let& arg : args )
			proto->add_args( arg );
		return t;
	}
}