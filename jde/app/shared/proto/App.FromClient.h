#pragma once
#include "../usings.h"

namespace Jde::App::FromClient{
	namespace PFromClient = Jde::App::Proto::FromClient;
	α AddSession( str domain, str loginName, ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->PFromClient::Transmission;
	α Exception( IException&& e )ι->PFromClient::Transmission;
	α GraphQL( str query, RequestId requestId )ι->PFromClient::Transmission;
	α Instance( str application, str instanceName, SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	α ToStatus( vector<string>&& details )ι->PFromClient::Status;
	α ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, App::Proto::FromClient::Status&& input )ι->PFromClient::Status;
	α Status( vector<string>&& details )ι->PFromClient::Transmission;
	//α ConnectTransmission( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	α Session( SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	α ToLogEntry( Logging::ExternalMessage m )ι->PFromClient::LogEntry;
	α AddStringField( PFromClient::Transmission& t, PFromClient::EFields field, uint32 id, str value )ι->void;
	α ToExternalMessage( const PFromClient::LogEntry& m )ι->Logging::ExternalMessage;
}