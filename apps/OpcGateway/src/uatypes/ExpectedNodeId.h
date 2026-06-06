#pragma once
#include <jde/opc/usings.h>
#include <jde/opc/uatypes/ExNodeId.h>

namespace Jde::Opc::Gateway{
	// Thin wrapper around std::expected<ExNodeId,StatusCode>.
	// libc++-22 declares std::expected's heterogeneous operator== at namespace scope (not as a
	// hidden friend), so using std::expected directly as a container's mapped_type makes any
	// iterator comparison (it!=end()) enumerate that operator== and recurse on its own constraint
	// ("satisfaction of constraint depends on itself").  Wrapping keeps the container's value_type
	// out of namespace std, so the broken overload is never considered.  This type forwards the
	// std::expected interface the callers use and converts to/from std::expected transparently.
	struct ExpectedNodeId final{
		using Expected = std::expected<ExNodeId,StatusCode>;
		ExpectedNodeId()ι = default;
		// Forward to the underlying expected so a single user-defined conversion still reaches it
		// (e.g. NodeId→ExNodeId→expected at a call site), matching how std::expected behaved before.
		template<class U> requires (!std::same_as<std::remove_cvref_t<U>,ExpectedNodeId> && std::constructible_from<Expected,U&&>)
		ExpectedNodeId( U&& x )ι : _value{ FWD(x) }{}
		α operator=( Expected x )ι->ExpectedNodeId&{ _value = std::move(x); return *this; }
		operator const Expected&()Ι{ return _value; }

		α has_value()Ι{ return _value.has_value(); }
		explicit operator bool()Ι{ return _value.has_value(); }
		α operator->()ι{ return _value.operator->(); }
		α operator->()Ι{ return _value.operator->(); }
		α& value()ι{ return _value.value(); }
		α& value()Ι{ return _value.value(); }
		α error()Ι{ return _value.error(); }
	private:
		Expected _value;
	};
}
