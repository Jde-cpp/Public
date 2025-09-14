#include <jde/framework.h>
#include <jde/framework/io/json.h>
//#include <jde/framework/coroutine/Await.h>
#include <jde/web/server/exports.h>
#include <jde/opc/usings.h>
//#include <jde/opc/exports.h>
#include <jde/opc/UAException.h>
#include <jde/app/shared/exports.h>

#include <jde/web/client/exports.h>
DISABLE_WARNINGS
#include <jde/web/client/proto/Web.FromServer.pb.h>
#include <jde/app/shared/proto/App.FromClient.pb.h>
#include <jde/app/shared/proto/App.FromServer.pb.h>
ENABLE_WARNINGS
// #include <jde/opc/types/proto/Opc.Common.pb.h>
// #include <jde/opc/types/proto/Opc.FromClient.pb.h>
// #include <jde/opc/types/proto/Opc.FromServer.pb.h>
