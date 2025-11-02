#include "LocalClient.h"
#include <jde/app/log/ProtoLog.h>

namespace Jde::App{
	sp<Server::LocalClient> _appClient = ms<Server::LocalClient>();
	α Server::AppClient()ι->sp<LocalClient>{ return _appClient; }
namespace Server{
	α LocalClient::InitLogging()ι->void{
		Logging::Add<ProtoLog>( "proto" );
		Logging::Init();
	}
}}