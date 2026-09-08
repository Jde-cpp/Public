# Overview

This site is the front end of the Jde OPC Gateway: it browses and secures the OPC UA servers the gateways connect to, manages who may do what, and watches the services that make it up.

## Sections

- [Gateways](/gateways) - the gateways, their server connections and the node trees behind them. See [Gateways](/help/gateways).
- [Access](/access) - users, groups, roles, resources and the permissions that tie them together. See [Access](/help/access).
- [Applications](/apps) - the running services, their logs and their log levels. See [Applications](/help/apps).

## Getting around

Search, favorites, breadcrumbs, themes and signing in are described under [Navigation](/help/navigation). The `?` button in the top bar opens the help topic for whichever page you are on.

## Services

| service | role |
|---|---|
| Application server | signs users in, holds the access database and collects the services' logs |
| OPC gateway | connects to OPC UA servers and serves their nodes to this site |
| OPC server | an OPC UA server of its own, fed by a PLC or by the emulator |
