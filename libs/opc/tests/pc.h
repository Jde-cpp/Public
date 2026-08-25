#pragma once
//open62541 first, and still deliberately - but not for the reason review #2 recorded.  UAString.h no longer uses
//UA_STRING as its include guard (it has #pragma once), so nothing shadows the vendor's macro any more.  The live reason
//is plainer: usings.h, opcHelpers.h and UAString.h all name UA_* types without including the vendor themselves.
#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/statuscodes.h>
#include <open62541/server.h>

#include <gtest/gtest.h>
#include <jde/fwk.h>
#include <jde/fwk/chrono.h>
#include <jde/fwk/io/json.h>
#include <jde/fwk/str.h>
#include <jde/db/Value.h>
#include <jde/opc/usings.h>
#include <jde/opc/UAException.h>
#include <jde/opc/uatypes/opcHelpers.h>

//Both have #pragma once now, so the "include exactly once, and only here" rule review #2 recorded is gone - a test .cpp
//may include either directly.  They stay in the PCH because every suite uses them.
#include <jde/opc/uatypes/DateTime.h>
#include <jde/opc/uatypes/LocalizedText.h>
