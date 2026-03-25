#pragma once

namespace Jde::Access{
	struct AccessException : IException{
		template<class... Args> AccessException( SL sl, UserPK executer, fmt::format_string<Args...> m, Args&& ...args )ι;
		β Log()Ι->void;
		α Move()ι->up<IException> override{ return mu<AccessException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		UserPK Executer;
	};
	template<class... Args> AccessException::AccessException( SL sl, UserPK executer, fmt::format_string<Args...> m, Args&& ...args )ι:
		IException{ sl, ELogLevel::Debug, m, FWD(args)... },
		Executer{ executer }
	{
		_tags = ELogTags::Access | ELogTags::Exception;
	}

}