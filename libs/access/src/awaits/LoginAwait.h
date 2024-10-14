#include <jde/access/awaits/LoginAwait.h>

namespace Jde::Access{
	LoginAwait::LoginAwait( vector<unsigned char> modulus, vector<unsigned char> exponent, string&& name, string&& target, string&& description, SL sl )ι:
			base{sl}, _modulus{ move(modulus) }, _exponent{ move(exponent) }, _name{ move(name) }, _target{ move(target) }, _description{ move(description) }
	{}

	α LoginAwait::LoginTask()ε->Jde::Task{
		auto modulusHex = Str::ToHex( (byte*)_modulus.data(), _modulus.size() );
		try{
			THROW_IF( modulusHex.size() > 1024, "modulus {} is too long. max length: {}", modulusHex.size(), 1024 );
			uint32_t exponent{};
			for( let i : _exponent )
				exponent = (exponent<<8) | i;
			vector<DB::Value> parameters = { modulusHex, exponent, underlying(EProviderType::Key) };
			let sql = "select e.id from um_entities e join um_users u on e.id=u.entity_id where u.modulus=? and u.exponent=? and e.provider_id=?";
			auto task = DB::ScalerCo<UserPK>( string{sql}, parameters );
			auto p = ( co_await task ).UP<UserPK>(); //gcc compile issue
			auto userPK = p ? *p : 0;
			if( !userPK ){
				parameters.push_back( move(_name) );
				parameters.push_back( move(_target) );
				parameters.push_back( move(_description) );
				DB::ExecuteProc( "um_user_insert_key(?,?,?,?,?,?)", move(parameters), [&userPK](let& row){userPK=row.GetUInt32(0);} );
			}
			Promise()->Resume( move(userPK), _h );
		}
		catch( IException& e ){
			Promise()->ResumeWithError( move(e), _h );
		}
	}

	α LoginAwait::Suspend()ι->void{
		LoginTask();
	}
