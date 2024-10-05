#include <jde/db/metadata/SchemaProc.h>
#include <boost/container/flat_map.hpp>
#include <jde/Str.h>
#include <jde/io/File.h>
#include <jde/db/Database.h>
#include <jde/db/DataSource.h>
#include "ddl/Syntax.h"
#include <jde/db/metadata/Table.h>
#include <jde/db/metadata/Column.h>
#include "ddl/TableDdl.h"
#include "ddl/Index.h"
#include "ddl/SchemaDdl.h"

#define let const auto
namespace Jde::DB{
	using boost::container::flat_map;
	using nlohmann::json;
	using nlohmann::ordered_json;
	struct IDataSource;

	constexpr ELogTags _tags{ ELogTags::Sql };
	string AbbrevName( sv schemaName ){
		auto fnctn = []( let& word )->string {
			std::ostringstream os;
			for( let ch : word ){
				if( (ch!='a' && ch!='e' && ch!='i' && ch!='o' && ch!='u') || os.tellp() == std::streampos(0) )
					os << ch;
			}
			return word.size()>2 && os.str().size()<word.size()-1 ? os.str() : string{ word };
		};
		let singular = DB::Schema::ToSingular( schemaName );
		let splits = Str::Split( singular, '_' );
		std::ostringstream name;
		for( uint i=1; i<splits.size(); ++i ){
			if( i>1 )
				name << '_';
			name << fnctn( splits[i] );
		}
		return name.str();
	}
}