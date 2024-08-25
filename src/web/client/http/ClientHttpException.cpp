#include <jde/web/client/http/ClientHttpException.h>
#include <jde/web/client/http/ClientHttpSession.h>

namespace Jde::Web::Client
{
	ClientHttpException::ClientHttpException( beast::error_code ec, ELogTags /*tags*/, ELogLevel level, SL sl )ι:
		ClientHttpException{ ec, {}, {}, level, sl }
	{}

	ClientHttpException::ClientHttpException( beast::error_code ec, str host, PortType port, ELogLevel level, SL sl )ι:
		CodeException{ static_cast<std::error_code>(ec), ELogTags::HttpClientWrite, level, sl },
		Host{ host },
		Port{ port }
	{}

	ClientHttpException::ClientHttpException( beast::error_code ec, sp<ClientHttpSession>& session, http::request<http::string_body>* _req, ELogLevel level, SL sl )ι:
		CodeException{ static_cast<std::error_code>(ec), ELogTags::HttpClientWrite, level, sl },
		Host{ session->Host },
		Target{ _req ? string{_req->target()} : "" },
		Port{ session->Port }
	{}


}