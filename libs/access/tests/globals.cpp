#include "globals.h"
#include <jde/framework/str.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/ql/ql.h>

#define let const auto

namespace Jde::DB{ struct IDataSource; struct AppSchema; }
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Test };
	static sp<DB::AppSchema> _schema;

	α Tests::SetSchema( sp<DB::AppSchema> schema )ι->void{_schema = schema;}
	α Tests::DS()ι->DB::IDataSource&{ return *_schema->DS(); }

	using namespace Json;
	α Tests::CreateUser( str name, uint providerId )ι->UserPK{
		let create = Ƒ( "{{ mutation createUser(  'input': {{'loginName':'{0}','target':'{0}','provider':{1},'name':'{0} - name','description':'{0} - description'}} ){{id}} }}", name, providerId );
		let createJson = QL::Query( Str::Replace(create, '\'', '"'), 0 );
		Trace{ _tags, "{}", serialize(createJson) };
		return AsNumber<UserPK>( createJson, "data/user/id" );//{"data":{"user":{"id":7}}}
	}
	α Tests::PurgeUser( UserPK userId )ι->void{
		let purge = Ƒ( "{{mutation purgeUser(\"id\":{}) }}", userId );
		let purgeJson = QL::Query( purge, 0 );
		Trace{ _tags, "purgeJson={}", serialize(purgeJson) };
	}
}
