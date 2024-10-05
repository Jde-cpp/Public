#include "ForeignKey.h"
#include <jde/db/metadata/Table.h>

namespace Jde::DB{
	α ForeignKey::Create( sv name, sv columnName, const DB::Table& pk, const DB::Table& foreignTable )ε->string{
		THROW_IF( pk.SurrogateKeys.size()!=1, "{} has {} columns in pk, <1 has not implemented", pk.Name, pk.SurrogateKeys.size() );
		std::ostringstream os;
		os << "alter table " << foreignTable.Name << " add constraint " << name << " foreign key(" << columnName << ") references " << pk.Name << "(" << Str::AddCommas(pk.SurrogateKeys) << ")";
		return os.str();
	}
}