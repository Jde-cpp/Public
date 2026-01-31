#pragma once
#include <jde/ql/types/MutationQL.h>
#include "../UAClient.h"

namespace Jde::Opc::Gateway{
	struct CallResponse final : UA_CallResponse{
		CallResponse( CallResponse&& x )ι;
		CallResponse( UA_CallResponse&& x )ι;
		~CallResponse(){ UA_CallResponse_clear(this); }
		α operator=( CallResponse&& x )ι->CallResponse&;
		α Validate( Handle uahandle, RequestId requestId, SL sl )ε->void;
		α ToJson()ι->jvalue;
	};

	struct ΓOPC CallAwait : TAwaitEx<CallResponse,TAwait<sp<UAClient>>::Task>, noncopyable{
		using base = TAwaitEx<CallResponse,TAwait<sp<UAClient>>::Task>;
		CallAwait( QL::MutationQL&& req, sp<Web::Server::SessionInfo> session, SRCE )ι:base{sl}, _ql{move(req)}, _session{move(session)}{}

		α Execute()ι->TAwait<sp<UAClient>>::Task override;
		α await_resume()ι->CallResponse;
	private:
		sp<UAClient> _client;
		QL::MutationQL _ql;
		RequestId 	_requestId{};
		sp<Web::Server::SessionInfo> _session;
	};
	struct JCallAwait : TAwaitEx<jvalue,TAwait<CallResponse>::Task>, noncopyable{
		using base = TAwaitEx<jvalue,TAwait<CallResponse>::Task>;
		JCallAwait( QL::MutationQL&& req, sp<Web::Server::SessionInfo> session, SRCE )ι:base{sl}, _ql{move(req)}, _session{move(session)}{}

		α Execute()ι->TAwait<CallResponse>::Task override;
	private:
		QL::MutationQL _ql;
		sp<Web::Server::SessionInfo> _session;
	};
}