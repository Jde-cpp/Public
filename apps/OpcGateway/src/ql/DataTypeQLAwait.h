#include <jde/ql/types/TableQL.h>

namespace Jde::Web::Server{ struct SessionInfo; }
namespace Jde::QL{ struct TableQL; }

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct DataTypeQLAwait final: TAwaitEx<jvalue,TAwait<sp<UAClient>>::Task>{
		using base = TAwaitEx<jvalue,TAwait<sp<UAClient>>::Task>;
		DataTypeQLAwait( QL::TableQL&& ql, sp<Web::Server::SessionInfo> sessionInfo, SRCE )ι: base{ sl }, _sessionInfo{move(sessionInfo)}, _ql{move(ql)}{};
		α Execute()ι->TAwait<sp<UAClient>>::Task override;
	private:
		sp<Web::Server::SessionInfo> _sessionInfo;
		QL::TableQL _ql;
	};
}