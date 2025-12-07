#pragma once

namespace Jde::QL{ struct Subscription; }
namespace Jde::Web::Server{
	struct IWebsocketSession;

	struct SubscribeLog final : Logging::ILogger, boost::noncopyable{
		SubscribeLog( jobject settings, const uint32& appPK, const uint32& instancePK )ε:ILogger{ move(settings) }, _appPK(appPK), _instancePK(instancePK){}
		Ω Init()ι->void;
		α Shutdown( bool terminate )ι->void override;
		α Add( sp<Web::Server::IWebsocketSession> session, vector<QL::Subscription>&& subs )ι->void;
		α Write( const Logging::Entry& m )ι->void override;
		α Write( const Logging::Entry& m, uint32 appPK, uint32 instancePK )ι->void override;
	private:
		uint32 _appPK;
		uint32 _instancePK;
	};
}