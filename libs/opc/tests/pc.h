#pragma once
//open62541 first, deliberately:  <jde/opc/uatypes/UAString.h> (reached through opcHelpers.h) uses UA_STRING as its
//include guard, which shadows the vendor's UA_STRING function and the UA_BYTESTRING macro that expands to it.  Vendor
//headers have to be parsed before that happens.  reviews/opc-review2.md, "below the cut".
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

//DateTime.h and LocalizedText.h have neither #pragma once nor an include guard, so they can only be included once per
//TU:  here, through the precompiled header, and never from a test .cpp.  reviews/opc-review2.md, "below the cut".
#include <jde/opc/uatypes/DateTime.h>
#include <jde/opc/uatypes/LocalizedText.h>
