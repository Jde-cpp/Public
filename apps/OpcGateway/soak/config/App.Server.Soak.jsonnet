// Soak overlay: production AppServer config with sync=true baked in (fresh per-run sqlite file gets schema+seed)
// and log tags flattened - the production tag map has many Trace/Debug tags, unbounded over 24h (no log rotation).
// Launch with -include=../../../AppServer/config/args/sqlite (relative to this file) and -arg path=<run>/db/app.db.
local base = (import '../../../AppServer/config/App.Server.jsonnet')(sync=true);
base + {
	//the soak client logs in with its own web cert (main.cpp -> SslSettings), issued under http.ssl.productName "Opc.Soak" - a test-only
	//product, so it is anchored in this overlay rather than in the production trustedCertDirs list (see the comment there).
	access+: { trustedCertDirs+: [ "$(ProgramData)/Jde-Cpp/Opc.Soak/ssl/certs" ] },
	logging+: {
		spd+: {
			tags: {
				default: "Information"
			}
		}
	}
}
