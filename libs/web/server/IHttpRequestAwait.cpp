#include <jde/web/server/IHttpRequestAwait.h>

namespace Jde::Web::Server{
	HttpTaskResult::HttpTaskResult( HttpTaskResult&& rhs )ι:
		Json( move(rhs.Json) ),
		Request{ move(rhs.Request) },
		Source{ move(rhs.Source) }
	{}

	α HttpTaskResult::operator=( HttpTaskResult&& rhs )ι->HttpTaskResult&{
		if( rhs.Request )
			Request.emplace( move(*rhs.Request) );
		else
			Request.reset();
		Json=move( rhs.Json );
		return *this;
	}
}