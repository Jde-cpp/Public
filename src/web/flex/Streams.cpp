#include <jde/web/flex/Streams.h>

namespace Jde::Web::Flex{
	α Streams::operator=( Streams&& rhs )ι->Streams&{
		Plain = move(rhs.Plain);
		Ssl = move(rhs.Ssl);
		return *this;
	}

	α Streams::AsyncWrite( http::message_generator&& m )ι->void{
		if( Plain )//TODO Move async_write to streams class and pass shared_from_this.  Implement certificates.
			beast::async_write( *Plain, move(m), beast::bind_front_handler(&Streams::OnWrite, shared_from_this()) );
		else
			beast::async_write( *Ssl, move(m), beast::bind_front_handler(&Streams::OnWrite, shared_from_this()) );
	}

	α Streams::OnWrite( beast::error_code ec, uint bytes_transferred )ι->void{
		boost::ignore_unused( bytes_transferred );
		if( ec )
			CodeException{ static_cast<std::error_code>(ec), ec.value()==(int)boost::beast::error::timeout ? ELogLevel::Debug : ELogLevel::Error };
  }

}
