#pragma once

namespace Jde::Iot{
	struct IotGraphQL;
	constexpr static string OpcServerTarget{ "OpcServerTests" };
	α CreateOpcServer()ι->uint;
	α PurgeOpcServer( OpcPK id=0 )ι->void;
	α SelectOpcServer( uint id=0 )ι->json;
	α AddHook()ι->void;
	α GetHook()ι->IotGraphQL*;
}