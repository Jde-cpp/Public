#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include <jde/framework.h>
#include <jde/framework/settings.h>
#include <jde/web/client/exports.h>
#include <jde/app/shared/exports.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
#include "../../../../Framework/source/Stopwatch.h"
