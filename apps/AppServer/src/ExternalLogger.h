#pragma once

namespace Jde::App{
	struct IApp;
	//Log server messages to db
	struct ExternalLogger final : Logging::ILogger{
		ExternalLogger( sp<IApp> appClient )ι:ILogger{ Settings::FindDefaultObject("/logging/proto") },_appClient{appClient}{}
		α Shutdown( bool /*terminate*/ )ι->void override{ _appClient.reset(); }
		α Name()Ι->string override{ return "externalLogger"; }
		α Write( Logging::Entry&& m )ι->void;
		α Write( const Logging::Entry& m )ι->void override;
	private:
		sp<IApp> _appClient;
	};
}