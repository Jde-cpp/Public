#pragma once

namespace Jde::Web::Flex{
	struct CancellationSignals final{
		std::list<net::cancellation_signal> sigs;
		mutex mtx;
		α emit(net::cancellation_type ct = net::cancellation_type::all)->void{
			lg _(mtx);
			for( auto & sig : sigs )
				sig.emit( ct );
		}
		α slot()->net::cancellation_slot{
			lg _(mtx);
			auto p = find_if( sigs, [](net::cancellation_signal & sig){ return !sig.slot().has_handler();} );
			return p == sigs.end() ? sigs.emplace_back().slot() : p->slot();
		}
	};
}
