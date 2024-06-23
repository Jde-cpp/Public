#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <jde/web/usings.h>
#include <jde/coroutine/Task.h>
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/io/AsioContextThread.h"
#include "../../../Framework/source/io/Socket.h"
#include "../../../Framework/source/io/ProtoClient.h"
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "../../../Framework/source/threading/InterruptibleThread.h"
DISABLE_WARNINGS
_Pragma("warning(disable: 5054)")
#include "../../../Framework/source/io/proto/messages.pb.h"
ENABLE_WARNINGS
#include "../../../Ssl/source/Ssl.h"