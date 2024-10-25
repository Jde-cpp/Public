#include "ForeignKey.h"
#include <jde/db/meta/Column.h>
#include <jde/db/meta/Table.h>
#include <jde/framework/str.h>

namespace Jde::DB{
	α ForeignKey::Create( sv name, sv columnName, const DB::Table& pk, const DB::Table& foreignTable )ε->string{
		THROW_IF( pk.SurrogateKeys.size()!=1, "{} has {} columns in pk, multiple has not implemented", pk.Name, pk.SurrogateKeys.size() );
		std::ostringstream os;
		os << "alter table " << foreignTable.DBName << " add constraint " << name << " foreign key(" << columnName << ") references " << pk.DBName << "(" << pk.GetPK()->Name << ")";
		return os.str();
	}
}