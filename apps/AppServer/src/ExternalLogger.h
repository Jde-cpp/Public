#pragma once

namespace Jde::App{
	struct IApp;
	//Log server messages to db
	struct ExternalLogger final : Logging::ILogger{
		ExternalLogger( sp<IApp> appClient )ι:ILogger{ Settings::FindDefaultObject("/logging/proto") },_appClient{appClient}{}
		α Name()ι->string override{ return "proto"; }
		α Write( Logging::Entry&& m )ι->void;
		α Write( const Logging::Entry& m )ι->void override;
	private:
		sp<IApp> _appClient;
	};
}