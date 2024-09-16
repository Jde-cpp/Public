cd ..
cmake -E copy_if_different^
  %JDE_DIR%/Framework/source/.build/.bin/Debug/Jde.dll^
  %JDE_DIR%/Public/src/web/client/.build/.bin/Debug/Jde.Web.Client.dll^
	%JDE_DIR%/Public/src/web/server/.build/.bin/Debug/Jde.Web.Server.dll^
	%JDE_DIR%/Public/src/crypto/.build/.bin/Debug/Jde.Crypto.dll^
	%JDE_DIR%/Public/src/app/shared/.build/.bin/Debug/Jde.App.Shared.dll^
  .build\.bin\Debug
	C:\Users\duffyj\source\repos\jde\Public\tests\web\.build\.bin\Debug\Jde.Web.Tests.exe