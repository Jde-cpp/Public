#include "Index.h"
#include <jde/db/meta/Table.h>
#include <jde/db/meta/Column.h>
#include <jde/db/generators/Syntax.h>

#define let const auto

namespace Jde::DB{
	Index::Index( sv indexName, sv tableName, bool primaryKey, vector<string> columns, bool unique, optional<bool> clustered )ι:
		Name{ indexName },
		TableName{ tableName },
		Columns{ move(columns) },
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

	α Index::Create( sv name, sv tableName, sv sqlTableName, const Syntax& syntax )Ι->string{
		let head = PrimaryKey
			? Ƒ( "alter table {} add constraint {}_{}{} primary key(", sqlTableName, Str::Replace(tableName, '.', '_'), name, Clustered || !syntax.SpecifyIndexCluster() ? "" : " nonclustered" )
			: Ƒ( "create {}{} index {} on \n{}(", Clustered && syntax.SpecifyIndexCluster() ? "clustered " : " ", Unique ? "unique" : "", name, sqlTableName );
		return Ƒ( "{}{})", head, Str::Join(Columns) );
	}

	α Index::GetConfig( const Table& t )ι->vector<Index>{
		vector<Index> indexes;
		if( t.SurrogateKeys.size() ){
			vector<string> names;
			bool haveNullColumn{};
			for( let& c : t.SurrogateKeys ){
				names.push_back( c->Name );
				if( c->IsNullable )
					haveNullColumn = true;
			}
			indexes.emplace_back( "pk", t.Name, !haveNullColumn, move(names) );
		}
		for( uint i=0; i<t.NaturalKeys.size(); ++i ){
			let name = t.NaturalKeys.size()==1 ? "nk" : Ƒ( "nk{}", i );
			indexes.emplace_back( name, t.Name, false, t.NaturalKeys[i] );
		}
		return indexes;
	}
}