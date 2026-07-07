#pragma once
#include "../exports.h"

namespace Jde::Web::Client{
	struct ΓWC ClientHttpResException final : Exception{
		ClientHttpResException( ClientHttpRes&& res, SRCE )ι:Exception{ sl }, _res{move(res)}{}

		α Status()Ι->http::status{ return _res.Status(); }

		α Move()ι->up<Exception> override{ return mu<ClientHttpResException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		ClientHttpRes _res;
	};
}