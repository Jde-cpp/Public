#pragma once

namespace Jde::Access{
	//TODO! remove
	struct IAccessIdentity{
		virtual ~IAccessIdentity()=0;
	};

	inline IAccessIdentity::~IAccessIdentity(){}
}
