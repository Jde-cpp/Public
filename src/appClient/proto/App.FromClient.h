
namespace Jde::App::Client::FromClient{
	Ξ StatusMessage( vector<string>&& details )ι->Proto::FromClient::Transmission{
		Proto::FromClient::Transmission t;
		auto status = t.add_messages()->mutable_status();
		*status->mutable_start_time() = IO::Proto::ToTimestamp( Logging::StartTime() );
		status->set_memory( IApplication::MemorySize() );
		status->set_server_min_log_level( (Jde::Proto::ELogLevel)Logging::MinLevel("db") );
		status->set_client_min_log_level( (Jde::Proto::ELogLevel)Logging::ClientMinLevel() );
		for_each( details, [status](auto&& detail){ status->add_details(move(detail)); } );
		return t;
	}
	Ξ ConnectMessage( SessionPK sessionId, RequestId requestId )ι->Proto::FromClient::Transmission{
		Proto::FromClient::Transmission t;
		auto req = t.add_messages();
		req->set_session_id( sessionId );
		req->set_request_id( requestId );
		return t;
	}
	Ξ SessionInfoMessage( SessionPK sessionId, RequestId requestId )ι->Proto::FromClient::Transmission{
		Proto::FromClient::Transmission t;
		auto req = t.add_messages();
		req->set_session_info( sessionId );
		req->set_request_id( requestId );
		return t;
	}
}