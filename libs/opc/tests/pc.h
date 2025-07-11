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

#include <jde/framework.h>
#include <jde/web/server/exports.h>
#include <jde/opc/usings.h>
#include <jde/opc/exports.h>
#include <jde/app/shared/exports.h>
#include "externals.h"
