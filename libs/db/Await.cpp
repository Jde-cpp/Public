#include <jde/db/awaits/DBAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde::DB
{
	α ICacheAwait::await_ready()ι->bool{
		_pValue = Cache::Get<void>( _name );
		return !!_pValue;
	}

	α ICacheAwait::await_resume()ι->AwaitResult{
		auto y = _pValue ? AwaitResult{ move(_pValue) } : base::await_resume();
		if( !_pValue && y.HasShared() )
			Cache::Set<void>( _name, y.SP<void>() );
		return y;
	}
}