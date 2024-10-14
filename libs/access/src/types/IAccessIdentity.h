#pragma once

namespace Jde::Access{
	struct IAccessIdentity{
		virtual ~IAccessIdentity()=0;
	};

	inline IAccessIdentity::~IAccessIdentity(){}
}
