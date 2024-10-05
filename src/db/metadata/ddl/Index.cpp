#include "Index.h"
#include <jde/db/metadata/Table.h>
#include "Syntax.h"

#define let const auto

namespace Jde::DB{
	Index::Index( sv indexName, sv tableName, bool primaryKey, const vector<string>* pColumns, bool unique, optional<bool> clustered )ι:
		Name{ indexName },
		TableName{ tableName },
		Columns{ pColumns ? *pColumns : vector<string>{} },
		Clustered{ clustered ? *clustered : primaryKey },
		Unique{ unique },
		PrimaryKey{ primaryKey }
	{}
	Index::Index( sv indexName, sv tableName, const Index& y )ι:
		Name{ indexName },
		TableName{ tableName },
		Columns{ y.Columns },
		Clustered{ y.Clustered },
		Unique{ y.Unique },
		PrimaryKey{ y.PrimaryKey }
	{}

	α Index::Create( sv name, sv tableName, const Syntax& syntax )Ι->string{
		string unique = Unique ? "unique" : "";
		std::ostringstream os;
		if( PrimaryKey )
			os << "alter table " << tableName << " add constraint " << name << (Clustered || !syntax.SpecifyIndexCluster() ? "" : " nonclustered") << " primary key(";
		else
			os << "create " << (Clustered && syntax.SpecifyIndexCluster() ? "clustered " : " ") << unique << " index "<< name << " on " << std::endl << tableName << "(";

		os << Str::AddCommas( Columns ) << ")";

		return os.str();
	}

	α Index::GetConfig( const Table& t )ι->vector<Index>{
		vector<Index> indexes;
		if( t.SurrogateKeys.size() ){
			Index i{"pk", t.Name, true, &t.SurrogateKeys};
			indexes.emplace_back( "pk", t.Name, true, &t.SurrogateKeys );
		}
		for( uint i=0; i<t.NaturalKeys.size(); ++i ){
			let name = Ƒ( "nk{}", i );
			indexes.emplace_back( name, t.Name, false, &t.NaturalKeys[i] );
		}
		return indexes;
	}
}