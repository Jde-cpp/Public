import { PropertyNames } from 'jde-framework';
import { ServerCnnctn, ServerCnnctnProps } from './ServerCnnctn.js';

export class ServerDesc{
	constructor( obj:ServerDescProps ){
		this.applicationUri = obj.applicationUri;
		this.productUri = obj.productUri;
		this.applicationName = obj.applicationName;
		this.applicationType = obj.applicationType;
		this.gatewayServerUri = obj.gatewayServerUri;
		this.discoveryProfileUri = obj.discoveryProfileUri;
		this.discoveryUrls = obj.discoveryUrls;
	}
	applicationUri: string;
	productUri: string;
	applicationName: string;
	applicationType: string;
	gatewayServerUri: string;
	discoveryProfileUri: string;
	discoveryUrls: string[];
	get accessResource():string{ return `${this.applicationName.substring(this.applicationName.indexOf('[')+1,this.applicationName.indexOf(']'))}`; }
}
export type ServerDescProps = Pick<ServerDesc, PropertyNames<ServerDesc>>;

export class Server extends ServerDesc{
	constructor( obj:ServerProps ){
		super( obj.desc );
		this.connection = new ServerCnnctn( obj.connection );
		this.policy = obj.policy;
		this.mode = obj.mode;
	}
	get opcTarget():string{ return this.connection.target; }
	connection: ServerCnnctn;
	policy: string;
	mode: string;
}
export type ServerProps = {
	connection: ServerCnnctnProps;
	desc: ServerDescProps;
	policy: string;
	mode: string;
};