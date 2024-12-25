#include <jde/db/awaits/ExecuteAwait.h>
#include <jde/db/IDataSource.h>

namespace Jde::DB{

	α ExecuteAwait::await_resume()ι->uint{
		return _isStoredProc
			? _ds->ExecuteProc( move(_sql.Text), _sql.Params, _sl )
			: _ds->Execute( move(_sql.Text), _sql.Params, _sl );
	}
}