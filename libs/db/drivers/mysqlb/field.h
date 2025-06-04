#pragma once
#include <jde/db/Value.h>

namespace Jde::DB::MySql{
	α ToField( const Value& v, SL sl )ε->mysql::field_view;
}