#pragma once
#include "../usings.h"
#include <jde/access/usings.h>
#include "App.FromClient.pb.h"

namespace Jde::Web{ struct Jwt; }
namespace Jde::App::FromClient{
	namespace PFromClient = Jde::App::Proto::FromClient;
	using StringTrans = string;
	α AddSession( str domain, str loginName, Access::ProviderPK providerPK, str userEndPoint, bool isSocket, RequestId requestId )ι->StringTrans;
	α Exception( exception&& e, RequestId requestId=0 )ι->PFromClient::Transmission;
	α Exception( string&& e, RequestId requestId )ι->PFromClient::Transmission;
	α Jwt( RequestId requestId )ι->StringTrans;
	α Login( Web::Jwt&& jwt, RequestId requestId )ι->StringTrans;
	α Query( string query, jobject variables, RequestId requestId, bool returnRaw=true )ι->string;
	α QueryResult( string&& result, RequestId requestId )ι->PFromClient::Transmission;
	α Instance( str application, str instanceName, SessionPK sessionId, RequestId requestId )ι->PFromClient::Transmission;
	α Session( SessionPK sessionId, RequestId requestId )ι->StringTrans;
	α Subscription( string&& query, jobject variables, RequestId requestId )ι->string;
	α LogEntries( vector<Logging::Entry>&& entries )ι->PFromClient::Transmission;
	α LogEntryClient( Logging::Entry&& m )ι->Log::Proto::LogEntryClient;
	α LogEntryFile( const Logging::Entry& m )ι->Log::Proto::LogEntryFile;
	α LogEntryFile( const Logging::Entry& m, App::ProgramPK appPK, App::ProgInstPK instancePK )ι->Log::Proto::LogEntryFileExternal;
	α FromLogEntry( Log::Proto::LogEntryClient&& m )ι->Logging::Entry;
	α ToString( uuid id, string&& value )ι->Log::Proto::String;
}