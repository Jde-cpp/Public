#include <jde/ql/types/TableQL.h>

namespace Jde::Web::Server{ struct SessionInfo; }
namespace Jde::QL{ struct TableQL; }

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct DataTypeQLAwait final: TAwait<jvalue>{
		DataTypeQLAwait( QL::TableQL&& ql, sp<UAClient> client, SRCE )ι: TAwait{ sl }, _client{move(client)}, _ql{move(ql)}{};
		α Suspend()ι->void override;
	private:
		sp<UAClient> _client;
		QL::TableQL _ql;
	};
}