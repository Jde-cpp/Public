// Soak overlay: production AppServer config with sync=true baked in (fresh per-run sqlite file gets schema+seed)
// and log tags flattened - the production tag map has many Trace/Debug tags, unbounded over 24h (no log rotation).
// Launch with -include=../../../AppServer/config/args/sqlite (relative to this file) and -arg path=<run>/db/app.db.
local base = (import '../../../AppServer/config/App.Server.jsonnet')(sync=true);
base + {
	logging+: {
		spd+: {
			tags: {
				default: "Information"
			}
		}
	}
}
