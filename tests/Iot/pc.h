#include <gtest/gtest.h>

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

#include <jde/TypeDefs.h>
#include <jde/Exception.h>
#include <jde/Log.h>
#include "../../../Framework/source/db/Database.h"
#include "../../src/web/Exports.h"
#include <jde/iot/Exports.h>
DISABLE_WARNINGS
#include <jde/iot/types/proto/IotCommon.pb.h>
#include <jde/iot/types/proto/IotFromServer.pb.h>
#include <jde/iot/types/proto/IotFromClient.pb.h>
ENABLE_WARNINGS
#include <jde/iot/TypeDefs.h>