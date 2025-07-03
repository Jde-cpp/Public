#include <jde/access/types/Resource.h>
#include <jde/db/Row.h>

#define let const auto
namespace Jde::Access{
	constexpr ELogTags _tags{ ELogTags::Access };

	Resource::Resource( DB::Row&& row )ι{
		PK = row.GetUInt16(0);
		Schema = move( row.GetString(1) );
		Target = move( row.GetString(2) );
		Filter = move( row.GetString(3) );
		IsDeleted = row.GetTimePointOpt(4);
	}
	Resource::Resource( ResourcePK pk, jobject j )ι:
		PK{ pk },
		Schema{ string{Json::FindDefaultSV(j, "schemaName")} },
		Target{ string{Json::FindDefaultSV(j, "target")} },
		Filter{ string{Json::FindDefaultSV(j, "criteria")} },
		IsDeleted{ Json::FindTimePoint(j, "deleted") }
	{}
	Resource::Resource( jobject j )ι:
		Resource{ Json::FindNumber<ResourcePK>(j, "id").value_or(0), move(j) }
	{}
}