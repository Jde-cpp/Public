#pragma once
#include <jde/http/IClientSocketSession.h>
#include <web/proto/test.pb.h>

namespace Jde::Web::Mock{
	using namespace Jde::Http;
	struct ClientSocketSession final : TClientSocketSession<Http::Proto::TestFromClient,Http::Proto::TestFromServer>{
		using base = TClientSocketSession<Http::Proto::TestFromClient,Http::Proto::TestFromServer>;
		ClientSocketSession( sp<net::io_context> ioc, optional<ssl::context>& ctx )ι;
		α OnRead( Http::Proto::TestFromServer&& transmission )ι->void override;
		// static sp<ClientSocketAwait> Create( net::io_context& ioc, ssl::context& ctx, const string& host, uint16 port, const string& target, uint version=11 );
		// void Connect( net::yield_context yield );
		// void Write( const string& message, net::yield_context yield );
		// string Read( net::yield_context yield );
		// void Close();
	};
}