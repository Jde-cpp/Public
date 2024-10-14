#pragma once

namespace Jde::Windows::Service
{
	void ReportStatus(unsigned long dwCurrentState, unsigned long dwWin32ExitCode, unsigned long dwWaitHint )ι;
	void ReportEvent( sv function )ι;
	void Main(unsigned long dwArgc, LPTSTR *lpszArgv )ι;
}