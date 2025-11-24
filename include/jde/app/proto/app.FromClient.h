#pragma once
#include "../usings.h"
#include <jde/access/usings.h>
#include "App.FromClient.pb.h"

namespace Jde::App::FromClient{
	namespace PFromClient = Jde::App::Proto::FromClient;
	using StringTrans = string;
	α AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->StringTrans;
	α Exception( exception&& e, RequestId requestId=0 )ι->PFromClient::Transmission;
	α Exception( string&& e, RequestId requestId )ι->PFromClient::Transmission;
	α Jwt( RequestId requestId )ι->StringTrans;
	α Query( string query, jobject variables, RequestId requestId, bool returnRaw=true )ι->string;
	α Instance( str application, str instanceName, SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	α ToStatus( vector<string>&& details )ι->PFromClient::Status;
	α ToStatus( AppPK appId, AppInstancePK instanceId, str hostName, App::Proto::FromClient::Status&& input )ι->PFromClient::Status;
	α Status( vector<string>&& details )ι->PFromClient::Transmission;
	α Session( SessionPK sessionId, RequestId requestId )ι->StringTrans;
	α Subscription( string&& query, jobject variables, RequestId requestId )ι->string;
	α LogEntries( vector<Logging::Entry>&& entries )ι->PFromClient::Transmission;
	α LogEntryClient( Logging::Entry&& m )ι->Log::Proto::LogEntryClient;
	α LogEntryFile( const Logging::Entry& m )ι->Log::Proto::LogEntryFile;
	α FromLogEntry( Log::Proto::LogEntryClient&& m )ι->Logging::Entry;
	α ToString( uuid id, string&& value )ι->Log::Proto::String;
}