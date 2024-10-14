#pragma once
#include "../exports.h"

namespace Jde::Web::Client{
	struct ΓWC ClientHttpResException final : IException{
		ClientHttpResException( ClientHttpRes&& res, SRCE )ι:IException{ sl }, _res{move(res)}{}

		α Status()Ι->http::status{ return _res.Status(); }

		α Move()ι->up<IException> override{ return mu<ClientHttpResException>(move(*this)); }
		α Ptr()ι->std::exception_ptr override{ return Jde::make_exception_ptr(move(*this)); }\
		[[noreturn]] α Throw()->void override{ throw move(*this); }
	private:
		ClientHttpRes _res;
	};
}