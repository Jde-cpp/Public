#pragma once
//#include <jde/log/Log.h>

namespace Jde::App{
	struct IApp;
	//Log server messages to db
	struct ExternalLogger final : Logging::IExternalLogger{
		ExternalLogger( sp<IApp> appClient )ι: _appClient{appClient}{}
		α Destroy( SL )ι->void override{};
		α Name()ι->string override{ return "db"; }
		α Log( Logging::ExternalMessage&& m, SRCE )ι->void;
		α Log( const Logging::ExternalMessage& m, const vector<string>* args=nullptr, SRCE )ι->void override;
		α SetMinLevel( ELogLevel level )ι->void override;
	private:
		sp<IApp> _appClient;
	};
}