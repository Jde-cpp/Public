import { InjectionToken } from '@angular/core';

//A node-scoped resource (schema "opc.<server>", criteria "ns=5;i=5005") is managed on a node page, which the opc library
//addresses by gateway, connection and browse path - none of which access knows.  The app provides the implementation
//(jde-opc's OpcNodeLinkResolver, app.config.ts); without one the Effective rights tab shows the criteria as text.
export type NodeLink = {
	route:string[];//a routerLink to the node's page
	name?:string;//the node's display name - what the link says, the criteria being the fallback
	path?:string;//its browse path, for the link's title
};
export interface NodeLinkResolver{
	//the node's link, or undefined when it cannot be placed:  no connection serves the schema, the node is outside the
	//Objects tree, the server is down.
	resolve( schema:string, criteria:string ):Promise<NodeLink|undefined>;
}
export const NODE_LINK_RESOLVER = new InjectionToken<NodeLinkResolver>( 'NodeLinkResolver' );
