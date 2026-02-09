#pragma once

namespace Jde::QL{

	struct IQLSession : noncopyable{
		IQLSession()ι=default;
		IQLSession( UserPK userPK )ι:UserPK{userPK}{}
		virtual ~IQLSession();
		UserPK UserPK;
	};
	inline IQLSession::~IQLSession(){}

	struct Creds{
		α Session()ι->sp<IQLSession>{
			auto p = std::get_if<sp<IQLSession>>(&Value);
			return p ? *p : nullptr;
		}
		α UserPK()ι->Jde::UserPK{
			auto userPK = std::get_if<Jde::UserPK>( &Value );
			if( auto p = userPK ? nullptr : Session(); p )
				userPK = &p->UserPK;
			return userPK ? *userPK : Jde::UserPK{};
		}
		variant<Jde::UserPK, sp<IQLSession>> Value;
	};
}
