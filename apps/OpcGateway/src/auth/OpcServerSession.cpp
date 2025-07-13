#include "OpcServerSession.h"
#define let const auto

namespace Jde::Opc::Gateway{
	concurrent_flat_map<SessionPK,flat_map<OpcClientNK,Credential>> _sessions;

	α Credential::operator==( const Credential& other )Ι->bool{
		bool equal{};
		if( _value.index() == other._value.index() ){
			switch( _value.index() ){
				case 0: equal = true; break;
				case 1: equal = get<User>(_value) == get<User>(other._value); break;
				case 2: equal = get<Token>(_value) == get<Token>(other._value); break;
				case 3: equal = get<Crypto::Certificate>(_value) == get<Crypto::Certificate>(other._value); break;
			}
		}
		return equal;
	}
	α Credential::operator<( const Credential& other )Ι->bool{
		optional<bool> less = _value.index() == other._value.index() ? optional<bool>{} : _value.index() < other._value.index();
		if( !less ){
			switch( _value.index() ){
				case 0: less = false; break;
				case 1: less = get<User>(_value) < get<User>(other._value); break;
				case 2: less = get<Token>(_value) < get<Token>(other._value); break;
				case 3: less = get<Crypto::Certificate>(_value) < get<Crypto::Certificate>(other._value); break;
			}
		}
		return *less;
	}

	α Credential::Type()Ι->ETokenType{
		ETokenType type;
		switch( _value.index() ){
			using enum ETokenType;
			case 0: type = Anonymous; break;
			case 1: type = Username; break;
			case 2: type = IssuedToken; break;
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
			case ETokenType::None: case ETokenType::Anonymous: _display = "anonymous"; break;
			case ETokenType::Username: _display = Ƒ("user: {}", LoginName()); break;
			case ETokenType::IssuedToken: _display = Ƒ("token: {:x}", (uint32)std::hash<string>{}(get<Token>(_value))); break;
			case ETokenType::Certificate: _display = Ƒ("cert: {:x}", get<Crypto::Certificate>(_value).hash32()); break;
		}
		return _display;
	}
}
namespace Jde::Opc{
	α Gateway::AddSession( SessionPK sessionId, OpcClientNK opcNK, Credential credential )ι->void{
		auto addCred = [&]( auto& sessionMap )ι{ sessionMap.second.try_emplace( move(opcNK), move(credential) ); };
		_sessions.try_emplace_and_visit( sessionId, addCred, addCred );
	}

	α Gateway::AuthCache( const Credential& cred, const OpcClientNK& opcNK )ι->optional<bool>{
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