#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <jde/web/usings.h>
#include <jde/coroutine/Task.h>
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "../../../Framework/source/io/AsioContextThread.h"
#include "../../../Framework/source/io/ProtoUtilities.h"
#include "../../../Framework/source/threading/InterruptibleThread.h"
#include "../../../Ssl/source/Ssl.h"