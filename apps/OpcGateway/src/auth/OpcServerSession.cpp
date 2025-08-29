#include "OpcServerSession.h"
#define let const auto

namespace Jde::Opc::Gateway{
	concurrent_flat_map<SessionPK,flat_map<ServerCnnctnNK,Credential>> _sessions;

	α Credential::operator==( const Credential& other )Ι->bool{
		bool equal{ Type() == other.Type() };
		if( equal ){
			switch( Type() ){
				using enum ETokenType;
				case None: case Anonymous: equal = true; break;
				case IssuedToken: equal = get<Gateway::Token>(_value) == get<Gateway::Token>(other._value); break;
				case Username: equal = get<User>(_value) == get<User>(other._value); break;
				case Certificate: equal = get<Crypto::PublicKey>(_value) == get<Crypto::PublicKey>(other._value); break;
			}
		}
		return equal;
	}
	α Credential::operator<( const Credential& other )Ι->bool{
		optional<bool> less{ Type() == other.Type() ? optional<bool>{} : Type() < other.Type() };
		if( !less ){
			switch( Type() ){
				using enum ETokenType;
				case None: case Anonymous: less = false; break;
				case IssuedToken: less = get<Gateway::Token>(_value) < get<Gateway::Token>(other._value); break;
				case Username: less = get<User>(_value) < get<User>(other._value); break;
				case Certificate: less = get<Crypto::PublicKey>(_value) < get<Crypto::PublicKey>(other._value); break;
			}
		}
		return *less;
	}

	α Credential::Type()Ι->ETokenType{
		ETokenType type;
		switch( _value.index() ){
			using enum ETokenType;
			case 0: type = Anonymous; break;
			case 1: type = IssuedToken; break;
			case 2: type = Username; break;
			case 3: type = Certificate; break;
		}
		return type;
	}
	α Credential::LoginName()Ι->str{ ASSERT(Type()==ETokenType::Username); return get<User>(_value).LoginName; }
	α Credential::Password()Ι->str{ ASSERT(Type()==ETokenType::Username); return get<User>(_value).Password; }
	α Credential::ToString()Ι->string{
		if( !_display.empty() )
			return _display;
		switch( Type() ){
			using enum ETokenType;
			case None: case Anonymous: _display = "anonymous"; break;
			case Username: _display = Ƒ("user: {}", LoginName()); break;
			case IssuedToken: _display = Ƒ("token: {:x}", (uint32)std::hash<string>{}(get<Gateway::Token>(_value))); break;
			case Certificate: _display = Ƒ("cert: {:x}", get<Crypto::PublicKey>(_value).hash32()); break;
		}
		return _display;
	}
}
namespace Jde::Opc{
	α Gateway::AddSession( SessionPK sessionId, ServerCnnctnNK opcNK, Credential credential )ι->void{
		auto addCred = [&]( auto& sessionMap )ι{ sessionMap.second.try_emplace( move(opcNK), move(credential) ); };
		_sessions.try_emplace_and_visit( sessionId, addCred, addCred );
	}

	α Gateway::AuthCache( const Credential& cred, const ServerCnnctnNK& opcNK, SessionPK sessionId )ι->optional<bool>{
		optional<bool> authenticated;
		_sessions.cvisit_while( [&]( let& sessionClients )ι{
			let& clients = sessionClients.second;
			if( auto p = clients.find(opcNK); p!=clients.end() ){
				auto& existingCred = p->second;
				if( existingCred.Type()!=cred.Type() )
					return true;
				if( existingCred.IsUser() ){
					if( existingCred.LoginName()==cred.LoginName() )
						authenticated = existingCred.Password()==cred.Password();
				}
				else if( existingCred==cred )
					authenticated = true;
			}
			return !authenticated.has_value();
		});
		if( authenticated && *authenticated )
			AddSession( sessionId, opcNK, cred );

		return authenticated;
	}

	α Gateway::Logout( SessionPK sessionId )ι->void{
		let erased = _sessions.erase( sessionId );
		if( erased )
			Trace( ELogTags::App, "Session {:x} erased.", sessionId );
		else
			Trace( ELogTags::App, "Session {:x} not found.", sessionId );
	}
	α Gateway::GetCredential( SessionPK sessionId, str opcId )ι->optional<Credential>{
		optional<Credential> cred;
		_sessions.cvisit( sessionId, [&cred, &opcId]( let& sessionMap )ι{
			if( auto p = sessionMap.second.find(opcId); p!=sessionMap.second.end() ){
				cred = p->second;
			}
		});
		return cred;
	}
}
namespace Jde::Opc{
	α Gateway::ToTokenType( UA_UserTokenType ua )ι->ETokenType{
		switch( ua ){
			using enum ETokenType;
			case UA_USERTOKENTYPE_ANONYMOUS: return Anonymous;
			case UA_USERTOKENTYPE_USERNAME: return Username;
			case UA_USERTOKENTYPE_CERTIFICATE: return Certificate;
			case UA_USERTOKENTYPE_ISSUEDTOKEN: return IssuedToken;
			default: return None;
		}
	}
}