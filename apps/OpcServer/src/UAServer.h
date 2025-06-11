#include <jde/opc/uatypes/Logger.h>

namespace Jde::Opc::Server {
	struct UAServer{
		UAServer()ε;
		~UAServer();

		α Run()ι->void;
		string ServerName;
	private:
		UA_ServerConfig _config;
		UA_Server* _ua{};
		optional<std::jthread> _thread;
		Logger _logger;
		friend struct ServerConfigAwait;
	};
}