//Live fields the gateway grafts onto the DB-backed ServerConnection rows (ql/OpcSessionsQLAwait.cpp: ServerCnnctnSessionsQLAwait answers
//`serverConnections{ … opcSessions{count} }`). `extend: true` appends these to the introspected server_connections columns instead of replacing them.
local UInt = { kind: "SCALAR", name: "UInt" };
local NonNullUInt = { kind: "NON_NULL", name: null, ofType: UInt };
local String = { kind: "SCALAR", name: "String" };
local NonNullString = { kind: "NON_NULL", name: null, ofType: String };
local serverConnection = {
	extend: true,
	fields: [
		{ name: "connectionStatus", type: { kind: "OBJECT", name: "ConnectionStatus" } },
		{ name: "opcSessions", type: { kind: "OBJECT", name: "OpcSessions" } },
		{ name: "opcConnections", type: { kind: "OBJECT", name: "OpcConnections" } }
	]
};
{
	ServerConnection: serverConnection, //the web client's canonical name (schemaWithEnums("ServerConnection")).
	serverConnections: serverConnection, //the query's name.
	OpcSessions: { //config-only types: no view, so __type(name:...) is answered from here alone.
		fields: [
			{ name: "count", type: NonNullUInt }
		]
	},
	OpcConnections: {
		fields: [
			{ name: "count", type: NonNullUInt }
		]
	},
	ConnectionStatus: { //Connected|Idle|Error, from the live UAClients and UAClient::ConnectErrors() - `error` carries why the last attempt failed.
		fields: [
			{ name: "name", type: NonNullString },
			{ name: "error", type: String }
		]
	}
}
