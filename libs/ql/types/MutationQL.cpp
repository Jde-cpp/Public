#include <jde/ql/types/MutationQL.h>
#include <jde/db/names.h>
#include <jde/db/meta/DBSchema.h>

namespace Jde::QL{
	α MutationQL::TableName()Ι->string{
		if( _tableName.empty() )
			_tableName = DB::Names::ToPlural<string>( DB::Names::FromJson<string,string>(JsonName) );
		return _tableName;
	}

	α MutationQL::InputParam( sv key )Ε->const jvalue&{
		return Json::AsValue( Input(), key );
	}

	α MutationQL::Input(SL sl)Ε->const jobject&{
		return Json::AsObject( Args, "input", sl );
	}
}