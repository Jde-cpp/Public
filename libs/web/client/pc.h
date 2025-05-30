
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <jde/framework.h>
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/web/client/exports.h>
#include "proto/Web.FromServer.pb.h"