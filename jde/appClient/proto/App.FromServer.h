#pragma once
#include "../usings.h"
#include "../exports.h"
#define var const auto
#define Φ ΓAC α
namespace Jde::App::FromServer{
	//namespace FromServerProto = Jde::App::Proto::FromServer;
	Φ ExecuteResponseTransmission( RequestId clientRequestId, string&& executionResult )ι->Proto::FromServer::Transmission;
	Φ TraceTransmission( LogPK id, AppPK appId, AppInstancePK instanceId, const Proto::FromClient::LogEntry& m, const vector<string>& args )ι->Proto::FromServer::Transmission;
	Φ Status( AppPK appId, AppInstancePK instanceId, str hostName, Proto::FromClient::Status&& input )ι->Proto::FromServer::Status;
	Ξ StatusTransmission( Proto::FromServer::Status status )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_status() = move( status );
		// status->set_application_id( (google::protobuf::uint32)appPK );
		// status->set_instance_id( (google::protobuf::uint32)appInstancePK );
		// status->set_host_name( IApplication::HostName() );
		// status->set_start_time( (google::protobuf::uint32)Clock::to_time_t(IApplication::StartTime()) );
		// status->set_db_log_level( (Jde::Proto::ELogLevel)serverLevel );
		// status->set_file_log_level( (Jde::Proto::ELogLevel)clientLevel );
		// status->set_memory( IApplication::MemorySize() );
		// status->add_values( Jde::format("Web Connections:  {}", sessionCount) );
		return t;
	}
	Ξ ExceptionTransmission( const IException& e )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		*t.add_messages()->mutable_exception() = e.what();
		return t;
	}
	Ξ ExecuteTransmission( RequestId sessionRequestId, UserPK userPK, string&& fromClientTransmission )ι->Proto::FromServer::Transmission{
		Proto::FromServer::Transmission t;
		auto toServer = t.add_messages();
		toServer->set_request_id( sessionRequestId );
		if( userPK ){
			auto customExecute = toServer->mutable_execute();
			customExecute->set_user_pk( userPK );
			*customExecute->mutable_transmission() = move( fromClientTransmission );
		}
		else
			toServer->set_execute_anonymous( move(fromClientTransmission) );
		return t;
	}
}

#undef var