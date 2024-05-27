#pragma once

namespace Jde::Iot{
	constexpr static string OpcServerTarget{ "OpcServerTests" };
	α CreateOpcServer()ι->uint;
	α PurgeOpcServer( uint id=0 )ι->void;
	α SelectOpcServer( uint id=0 )ι->json;
}