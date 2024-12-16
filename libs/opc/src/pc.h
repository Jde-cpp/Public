#pragma once
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
#include <jde/framework/str.h>
#include <jde/framework/coroutine/Task.h>
#include "../../../../Framework/source/DateTime.h"
#include "../../../../Framework/source/coroutine/Alarm.h"
#include "../../../../Framework/source/coroutine/Awaitable.h"
#include <jde/crypto/OpenSsl.h>
#include <jde/db/meta/AppSchema.h>
#include <jde/db/meta/View.h>

#include <jde/opc/exports.h>
DISABLE_WARNINGS
//#include <jde/opc/types/proto/Opc.Common.pb.h>
//#include <jde/opc/types/proto/Opc.FromServer.pb.h>
//#include <jde/opc/types/proto/Opc.FromClient.pb.h>
ENABLE_WARNINGS
#include <jde/opc/uatypes/helpers.h>
#include <jde/opc/uatypes/UAException.h>
