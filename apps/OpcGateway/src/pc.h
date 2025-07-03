#pragma once
#include <boost/beast.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_highlevel_async.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <jde/opc/usings.h>
#include <jde/framework.h>
//#include <jde/App.h>
#include <jde/framework/str.h>
#include <jde/crypto/OpenSsl.h>
#include "../../Framework/source/DateTime.h"
#include "../../Framework/source/coroutine/Alarm.h"
//#include "../../Framework/source/db/GraphQL.h"
//#include "../../Framework/source/db/Database.h"
//#include "../../Framework/source/io/AsioContextThread.h"

#include <jde/opc/async/SessionAwait.h>
DISABLE_WARNINGS
//#include <jde/opc/types/proto/FromServer.pb.h>
#include <jde/opc/types/proto/Opc.Common.pb.h>
#include <jde/opc/types/proto/Opc.FromClient.pb.h>
#include <jde/opc/types/proto/Opc.FromServer.pb.h>
ENABLE_WARNINGS
//#include <jde/opc/types/FromServer.h>
#include <jde/opc/uatypes/UAClientException.h>