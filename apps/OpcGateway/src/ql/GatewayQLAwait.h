#pragma once
#include <jde/ql/types/RequestQL.h>
#include <jde/web/server/IHttpRequestAwait.h>
#include <jde/ql/IQLAwaitExe.h>

namespace Jde::Opc::Gateway{
	struct UAClient;
	struct IGatewayQLAwait : noncopyable{
	protected:
		α GetClient( QL::IQLAwaitExe* await )ι->TAwait<sp<UAClient>>::Task;
		sp<UAClient> _client;
	};
	struct GatewayQLAwait final : QL::IQLTableAwaitExe, IGatewayQLAwait{
		using base = QL::IQLTableAwaitExe;
		GatewayQLAwait( QL::TableQL q, QL::Creds creds, SRCE )ι:base{move(q), move(creds), sl}{}
		Ω Test( QL::TableQL& q, QL::Creds executer, SL sl )->up<TAwait<jvalue>>;
		α Suspend()ι->void override{ GetClient( this ); }
	private:
		α Query()ι->TAwait<jvalue>::Task override;
		α ConnectionAttributes( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α ServerDescription( QL::TableQL&& q, sp<UAClient> client )ι->jobject;
		α SecurityPolicyUri( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
		α SecurityMode( QL::TableQL&& q, sp<UAClient> client )ι->jvalue;
	};
	struct GatewayQLMAwait final : QL::IQLTableMutationExe, IGatewayQLAwait{
		using base = QL::IQLTableMutationExe;
		GatewayQLMAwait( QL::MutationQL&& q, QL::Creds creds, SRCE )ι:base{move(q), move(creds), sl}{}
		Ω Test( QL::MutationQL& q, QL::Creds executer, SL sl )->up<TAwait<jvalue>>;
		α Suspend()ι->void override{ GetClient( this ); }
	private:
		α Query()ι->TAwait<jvalue>::Task override;
	};
}