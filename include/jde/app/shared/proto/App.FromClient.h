#pragma once
#include "../usings.h"
#include "../exports.h"
#include <jde/access/usings.h>
#include "App.FromClient.pb.h"

#define Φ ΓAS auto
namespace Jde::App::FromClient{
	namespace PFromClient = Jde::App::Proto::FromClient;
	using StringTrans = string;
	Φ AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->StringTrans;
	Φ Exception( exception&& e, RequestId requestId=0 )ι->PFromClient::Transmission;
	Φ Exception( string&& e, RequestId requestId )ι->PFromClient::Transmission;
	Φ Jwt( RequestId requestId )ι->StringTrans;
	Φ Query( string query, RequestId requestId, bool returnRaw=true )ι->PFromClient::Transmission;
	Φ Instance( str application, str instanceName, SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	Φ ToStatus( vector<string>&& details )ι->PFromClient::Status;
	Φ ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, App::Proto::FromClient::Status&& input )ι->PFromClient::Status;
	Φ Status( vector<string>&& details )ι->PFromClient::Transmission;
	Φ Session( SessionPK sessionId, RequestId requestId )ι->StringTrans;
	Φ Subscription( string&& query, RequestId requestId )ι->PFromClient::Transmission;
	Φ ToLogEntry( Logging::Entry m )ι->PFromClient::LogEntry;
	Φ FromLogEntry( PFromClient::LogEntry&& m )ι->Logging::Entry;
	Φ AddStringField( PFromClient::Transmission& t, PFromClient::EFields field, uuid id, str value )ι->void;
}
#undef Φ