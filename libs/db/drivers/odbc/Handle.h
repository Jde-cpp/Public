#pragma once
#include <boost/noncopyable.hpp>
#include <jde/fwk/settings.h>

namespace Jde::DB{ struct IRow; }
namespace Jde::DB::Odbc
{
	struct FetchAwaitable; struct OdbcDataSource; struct ExecuteAwaitable;

	struct HandleSession : noncopyable{
		HandleSession()ε;
		HandleSession( sv connectionString )ε;
		HandleSession( HandleSession&& rhs )ι:_hStatement{rhs._hStatement}{ rhs._hStatement=nullptr; };
		~HandleSession();
		virtual auto Connect( sv connectionString )ε->void;
		operator HDBC()Ι{ return _hStatement; }
	protected:
		HDBC _hStatement{nullptr};
		//HandleEnvironment _hEnv;
	};
	struct HandleStatement : noncopyable{
		HandleStatement( string connectionString )ε;
		HandleStatement( HandleStatement&& rhs )ι:_hStatement{move(rhs._hStatement)}, _session{move(rhs._session)}{ rhs._hStatement=nullptr; };
		~HandleStatement();
		operator SQLHSTMT()Ι{ return _hStatement; }
	private:
		SQLHSTMT _hStatement{nullptr};
		HandleSession _session;
	};
}