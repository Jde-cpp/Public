#pragma once

namespace Jde::Access{
	struct AccessException : Exception{
		AccessException( AccessException&& from )ι=default;
		template<class... Args> AccessException( SL sl, UserPK executer, fmt::format_string<Args...> m, Args&& ...args )ι;
		~AccessException(){ Log(); SetLevel( ELogLevel::NoLog ); }
		α Log()Ι->void override;
		α Move()ι->up<Exception> override{ return mu<AccessException>(move(*this)); }
		[[noreturn]] α Throw()->void override{ throw move(*this); }
		UserPK Executer;
	};
	template<class... Args> AccessException::AccessException( SL sl, UserPK executer, fmt::format_string<Args...> m, Args&& ...args )ι:
		Exception{ sl, {DefaultExceptionLevel, ELogTags::Access | ELogTags::Exception}, m, FWD(args)... },
		Executer{ executer }
	{}
}