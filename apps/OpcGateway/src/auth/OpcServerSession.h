#pragma once
#include <jde/crypto/OpenSsl.h>

namespace Jde::Opc::Gateway{
	struct User{
		α operator==( const User& other )Ι{ return LoginName == other.LoginName && Password == other.Password; }
		α operator<( const User& other )Ι{ return LoginName == other.LoginName ? Password < other.Password : LoginName < other.LoginName; }
		string LoginName;
		string Password;
	};
	using Token = string;
	enum class ETokenType : uint8{
		None=0,
		Anonymous=1,
		Username=2,
		Certificate=4,
		IssuedToken=8
	};
	α ToTokenType( UA_UserTokenType ua )ι->ETokenType;
	struct Credential{
		Credential()ι{}
		Credential( User user )ι:_value{move(user)}{}
		Credential( Token token )ι:_value{move(token)}{}
		Credential( Crypto::PublicKey key )ι:_value{move(key)}{}
		α operator==( const Credential& other )Ι->bool;

		α Token()Ι->const Gateway::Token&{ return get<Gateway::Token>( _value ); }
		α Type()Ι->ETokenType;
		α UserPK()Ι->Jde::UserPK{ return _userPK; }
		α SetUserPK( Jde::UserPK userPK )ι->void{ _userPK = userPK; }
		α IsUser()Ι{ return Type()==ETokenType::Username; }
		α LoginName()Ι->str;
		α Password()Ι->str;
		α ToString()Ι->string;
		α operator<( const Credential& other )Ι->bool;
	private:
		variant<nullptr_t, Gateway::Token, User, Crypto::PublicKey> _value;
		mutable string _display;
		Jde::UserPK _userPK;
	};
	α AddSession( SessionPK sessionId, ServerCnnctnNK opcNK, Credential credential )ι->void;
	α AuthCache( const Credential& credential, const ServerCnnctnNK& opcNK, SessionPK sessionId )ι->optional<bool>;
	α Logout( SessionPK sessionId )ι->void;
	α GetCredential( SessionPK sessionId, str opcId )ι->optional<Credential>;
}