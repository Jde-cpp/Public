#pragma once
#include <jde/opc/uatypes/Logger.h>
#include "UAAccess.h"

namespace Jde::Opc::Server {
	struct UAConfig final : UA_ServerConfig {
		UAConfig()ε;
	private:
		α SetAccessControl()ι;
		α SetConfig( PortType port, ByteStringPtr&& certificate, const ByteStringPtr&& privateKey )ε->void;
		α SetupSecurityPolicies( fs::path&& certificateFile )ε->void;
		α AddSecurityPolicies( ByteStringPtr&& certificate, const ByteStringPtr&& privateKey )ε->void;
		Logger _logger;
	};
}