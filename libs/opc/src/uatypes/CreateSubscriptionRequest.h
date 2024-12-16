#pragma once

namespace Jde::Opc{
	struct CreateSubscriptionRequest : UA_CreateSubscriptionRequest{
		CreateSubscriptionRequest()ι:
	    requestedPublishingInterval{500.0},
	    requestedLifetimeCount{10000},
	    requestedMaxKeepAliveCount{10},
	    maxNotificationsPerPublish{0},
	    publishingEnabled{true},
	    priority{0}
		{}
	};
}