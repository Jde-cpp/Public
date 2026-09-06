#pragma once
#include <jde/db/Row.h>
#include "Binding.h"

namespace Jde::DB::Odbc{
	//The fetched row, from the bound columns' current values.  (C: the OdbcRow class this replaces carried eleven typed
	//accessors, an operator[] cache and an index that nothing read - ToRow was its only caller.)
	α ToRow( const vector<up<Binding>>& bindings )ι->Row;
}