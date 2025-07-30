#include <jde/framework/io/json.h>
#include <jde/crypto/OpenSsl.h>
#include "Auth.h"
#include "../../src/auth/CertAwait.h"
#include "../../src/auth/OpcServerSession.h"
#include "../../src/GatewayAppClient.h"

#define let const auto

namespace Jde::Opc::Gateway::Tests{
	constexpr ELogTags _tags{ ELogTags::Test };

	class CertTests : public Auth{
	protected:
		CertTests()ι:Auth{ETokenType::Certificate}{}
		~CertTests()override{}
		Ω SetUpTestCase()ε->void;
		α TearDown()ι->void override{}
		Ω TearDownTestSuite();

		α Connect( atomic_flag& flag )ι->CertAwait::Task;
		optional<Credential> _cred;
		up<IException> _exception;
	};

	α CertTests::SetUpTestCase()ε->void{

	}
	α CertTests::TearDownTestSuite(){
		Auth::TearDownTestSuite();
	}

	α CertTests::Connect( atomic_flag& flag )ι->CertAwait::Task{
		try{
			co_await CertAwait{ Client->Target, "localhost", true };
		}
		catch( IException& e ){
			_exception = e.Move();
		}
		flag.test_and_set();
		flag.notify_all();
		Trace{ _tags, "notify_all" };
	}

	TEST_F( CertTests, Authenticate ){
		string opcId{ Client->Target };
		atomic_flag a,b,c;
		Connect( a );
		a.wait( false );
		Trace{ _tags, "Call b" };
		Connect( b );
		Trace{ _tags, "Call c" };
		Connect( c );
		b.wait( false );
		Trace{ _tags, "b returned" };
		c.wait( false );
		Trace{ _tags, "c returned" };
		EXPECT_FALSE( _exception );
	}

	TEST_F( CertTests, Authenticate_Bad ){
		let root = IApplication::ApplicationDataFolder();
		let working = root/"ssl";
		if( fs::exists(working) && !fs::exists(root/"ssl_backup") )
			fs::rename( working, root/"ssl_backup" );
		if( fs::exists(root/"ssl_badTest") )
			fs::rename( root/"ssl_badTest", working );
		else{
			Crypto::CryptoSettings settings{ jobject{} };
			settings.CreateDirectories();
			Crypto::CreateKeyCertificate( settings );
		}
		atomic_flag flag;
		Connect( flag );
		flag.wait( false );
		fs::rename( working, root/"ssl_badTest" );
		fs::rename( root/"ssl_backup", working );

		EXPECT_TRUE( _exception );
		EXPECT_TRUE( _exception && string{_exception->what()}.contains("BadIdentityTokenInvalid") );
		Debug( _tags, "{}", _exception ? _exception->what() : "Error no exception." );
	}
}