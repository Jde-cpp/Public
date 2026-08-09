#include <jde/app/proto/common.h>

namespace Jde::App{
	static_assert( (int)Jde::Proto::Jde==(int)Jde::Exception::ECategory::Jde && (int)Jde::Proto::DB==(int)Jde::Exception::ECategory::DB );//Common.proto ECategory mirrors the fwk enum.

	α ProtoUtils::ToException( Exception&& e )ι->Jde::Proto::Exception{
		Jde::Proto::Exception proto;
		proto.set_what( e.what() );
		proto.set_code( e.Code() );
		proto.set_status_code( e.HttpStatus() );
		proto.set_category( (Jde::Proto::ECategory)e.Category() );
		proto.set_category_code( e.CategoryCode() );
		return proto;
	}
	α ProtoUtils::ToQuery( string&& text, jobject&& variables, bool returnRaw )ι->Jde::Proto::Query{
		Jde::Proto::Query query;
		query.set_text( move(text) );
		query.set_variables( serialize(variables) );
		query.set_return_raw( returnRaw );
		return query;
	}
}