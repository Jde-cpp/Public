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
#include <jde/iot/TypeDefs.h>
#include <jde/Exports.h>
#include <jde/log/Log.h>
#include <jde/App.h>
#include <jde/Str.h>
#include <jde/coroutine/Task.h>
#include <jde/crypto/OpenSsl.h>
#include "../web/Exports.h"
#include "../../../Framework/source/DateTime.h"
#include "../../../Framework/source/coroutine/Alarm.h"
#include "../../../Framework/source/coroutine/Awaitable.h"
#include "../../../Framework/source/db/Database.h"
#include <jde/iot/Exports.h>
DISABLE_WARNINGS
#include <jde/iot/types/proto/IotCommon.pb.h>
#include <jde/iot/types/proto/IotFromServer.pb.h>
#include <jde/iot/types/proto/IotFromClient.pb.h>
ENABLE_WARNINGS
#include <jde/iot/uatypes/helpers.h>
#include <jde/iot/uatypes/UAException.h>
