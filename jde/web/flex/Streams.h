#pragma once
#include "../usings.h"

namespace Jde::Web::Flex{
	struct Streams final : std::enable_shared_from_this<Streams>{
		Streams( up<StreamType>&& plain )ι:Plain{ move(plain) }{}
		Streams( up<beast::ssl_stream<StreamType>>&& ssl )ι:Ssl{ move(ssl) }{}
		Streams( Streams&& rhs ):Plain{ move(rhs.Plain) }, Ssl{ move(rhs.Ssl) }{}
		α operator=( Streams&& rhs )ι->Streams&;
		α AsyncWrite( http::message_generator&& m )ι->void;
	private:
		α OnWrite( beast::error_code ec, uint bytes_transferred )ι->void;
		up<StreamType> Plain;
		up<beast::ssl_stream<StreamType>> Ssl;
	};
}