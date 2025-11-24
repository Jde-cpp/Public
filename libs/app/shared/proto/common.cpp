#include <jde/app/proto/common.h>

namespace Jde::App{
	α ProtoUtils::ToException( IException&& e )ι->Jde::Proto::Exception{
		Jde::Proto::Exception proto;
		proto.set_what( e.what() );
		proto.set_code( e.Code );
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