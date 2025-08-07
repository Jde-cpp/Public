DISABLE_WARNINGS
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <gtest/gtest.h>
#include <jde/app/shared/exports.h>
#include <jde/app/shared/proto/Common.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
ENABLE_WARNINGS