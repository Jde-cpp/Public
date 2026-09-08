# Gateways

A **gateway** is a running OPC gateway service. It holds **server connections**, one per OPC UA server it talks to, and each connection exposes that server's address space as a tree of **nodes**.

## Browsing

The [Gateways](/gateways) page lists the gateways; a gateway lists its connections; a connection opens the root of its node tree.

A node page has two tabs.

**Children** lists the node's child nodes with their current values. Click an object node to descend; the breadcrumb trail leads back up. The refresh button re-reads the values, the view buttons choose the columns, and **Access** opens the node's rights.

**Permissions** shows who may read and write the node.

The address of a node is in the page's url, so a node page can be bookmarked or made a favorite. Nodes that have been browsed in this session also turn up in the navbar search.

## Instances

Under [Applications](/apps), a gateway instance's page has three tabs: its **Connections**, its **Logs**, and its **Log Settings**, which change the running service's log levels. An OPC server instance's page has **Logs** and **Log Settings**.

## Certificates

A gateway checks a server's certificate against its trusted-certificate directory before it connects. A connection that stays in the connecting state usually means the server's certificate has not been placed there, or the server has not yet trusted the gateway's.

The repository ships a PLC emulator (`apps/OpcServer/emulator`) that feeds the OPC server with changing values, for trying the pages out without plant equipment.
