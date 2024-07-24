#include <jde/app/client/AppClient.h>
// #include "../../../../Framework/source/io/ProtoUtilities.h"
// #include "../../../../Framework/source/db/DataSource.h"
#include <jde/app/client/AppClientSocketSession.h>
//#include <jde/app/shared/proto/App.FromClient.h>

#define var const auto

namespace Jde{
//	bool _isAppServer{false};
	UserPK _appServiceUserPK{0};
//	sp<DB::IDataSource> _datasource;
//	α App::IsAppServer()ι->bool{ return _isAppServer; }
//	α App::SetIsAppServer( bool x )ι->void{ _isAppServer = x; }
//	α App::Datasource()ι->sp<DB::IDataSource>{ return _datasource; }
//	α App::SetDatasource( sp<DB::IDataSource> datasource )ι->void{ _datasource = datasource; }
	// α App::AppPK()ι->AppPK{ return _appId; }
	// α App::SetAppPK( AppPK x )ι->void{ _appId=x; }
	// α App::InstanceId()ι->AppInstancePK{ return _instanceId;}
	// α App::SetInstanceId( AppInstancePK x )ι->void{ _instanceId=x; }
}

namespace Jde::App{
	α Client::AppServiceUserPK()ι->UserPK{ return _appServiceUserPK; }
//	α Client::SetAppServiceUserPK(UserPK x)ι->void{ _appServiceUserPK = x; }

	function<vector<string>()> _statusDetails = []->vector<string>{ return {}; };
  α Client::SetStatusDetailsFunction( function<vector<string>()>&& f )ι->void{ _statusDetails = f; }

	#define IF_OK if( auto pSession = IApplication::ShuttingDown() ? nullptr : AppClientSocketSession::Instance(); pSession )
	// update immediately
	α Client::UpdateStatus()ι->void{
		IF_OK
			pSession->Write( FromClient::Status(_statusDetails()) );
	}
namespace Client{
	LoginAwait::LoginAwait( Crypto::Modulus mod, Crypto::Exponent exp, string userName, string userTarget, string myEndpoint, string description, SL sl )ι:
		base{sl},
		_jwt{ move(mod), exp, move(userName), move(userTarget), move(myEndpoint), move(description), {} }
	{}
	α LoginAwait::Execute()ι->void{

	}
}}