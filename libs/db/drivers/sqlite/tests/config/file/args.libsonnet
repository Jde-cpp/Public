{
	local args = self,
	runtimeDir: "$(RUNTIME_DIR)",
	dbServers: {
		sqllite: {
			path: args.runtimeDir+"/sqlite-tests.db"
		}
	}
}
