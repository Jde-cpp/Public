#include "UAClientException.h"
#include "../UAClient.h"
#include "jde/opc/uatypes/Logger.h"

namespace Jde::Opc::Gateway{
	α httpStatus( StatusCode sc )ι->Jde::EHttpStatus{
		switch( sc ){
			case UA_STATUSCODE_BADIDENTITYTOKENREJECTED: return EHttpStatus::Forbidden;
			case UA_STATUSCODE_BADCONNECTIONREJECTED: return EHttpStatus::BadGateway;
			default: return EHttpStatus::InternalServerError;
		}
	}
	UAClientException::UAClientException( StatusCode sc, Handle uaHandle, RequestId requestId, SL sl, ELogLevel level )ι:
		UAException{ sc, {}, {.Level=level, .HttpStatus=httpStatus(sc)}, sl },
		_handle{ uaHandle },
		_requestId{ requestId }
	{}

	UAClientException::UAClientException( StatusCode sc, sp<UAClient> client, RequestId requestId, SL sl, ELogLevel level )ι:
		UAClientException{ sc, client ? client->Handle() : 0, requestId, sl, level }{
		if( sc==UA_STATUSCODE_BADSERVERNOTCONNECTED )//TODO! (review #27) Mutating the global client registry inside an exception ctor is surprising and is the double-remove feeder behind #2. The intended owner is the retry path (UAClient::Retry/RetryVoid both RemoveClient for exactly these connection-loss codes) but that path is dead code (#28). This is currently the SOLE live removal on BADSERVERNOTCONNECTED, so it can't simply be deleted. Deferred: relocate removal to an explicit throw-site/handler when the retry path is revived and the client lifecycle is reworked (#3 threading redesign).
			UAClient::RemoveClient( move(client) );
	}

	UAClientException::UAClientException( StatusCode sc, Handle uaHandle, string description, SL sl )ι:
		UAException{ sc, description, {.HttpStatus=httpStatus(sc)}, sl },
		_userMessage{ move(description) }
	{}

	α UAClientException::what()const noexcept->const char*{
		if( _what.empty() ){
			if( _requestId )
				_what = Ƒ( "[{:x}.{:x}]{}", _handle, _requestId, Exception::what() );
			else
				_what = Ƒ( "[{:x}]{}", _handle, Exception::what() );
		}
		return _what.c_str();
	}
	α UAClientException::UserMessage()Ι->const string&{
		if( _userMessage.empty() )
			_userMessage = _userMessage.size() ? Ƒ("{} - {}", UAException::UserMessage(), _userMessage) : UAException::UserMessage();
		return _userMessage;
	}
}