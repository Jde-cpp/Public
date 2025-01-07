#pragma once
#include "../usings.h"
#include "../exports.h"
#include <jde/access/usings.h>

#define Φ ΓAS auto
namespace Jde::App::FromClient{
	namespace PFromClient = Jde::App::Proto::FromClient;
	Φ AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->PFromClient::Transmission;
	Φ Exception( IException&& e )ι->PFromClient::Transmission;
	Φ Query( string query, RequestId requestId )ι->PFromClient::Transmission;
	Φ Instance( str application, str instanceName, SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	Φ ToStatus( vector<string>&& details )ι->PFromClient::Status;
	Φ ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, App::Proto::FromClient::Status&& input )ι->PFromClient::Status;
	Φ Status( vector<string>&& details )ι->PFromClient::Transmission;
	Φ Session( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	Φ Subscription( string&& query, RequestId requestId )ι->PFromClient::Transmission;
	Φ ToLogEntry( Logging::ExternalMessage m )ι->PFromClient::LogEntry;
	Φ AddStringField( PFromClient::Transmission& t, PFromClient::EFields field, uint32 id, str value )ι->void;
	Φ ToExternalMessage( const PFromClient::LogEntry& m )ι->Logging::ExternalMessage;
}
#undef Φ