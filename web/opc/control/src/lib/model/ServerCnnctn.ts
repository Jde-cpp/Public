import { ITargetRow, Mutation, MutationType, PropertyNames, TargetRow, TargetRowProps  } from "jde-framework";
import { toBrowse } from "./types";
import { Server } from "./Server";

export type CnnctnPK = number;
export type CnnctnTarget = string;
export class ServerCnnctn extends TargetRow<ServerCnnctn>{
	constructor( obj:ServerCnnctnProps ){
		super(ServerCnnctn.typeName, obj);
		this.url = obj.url;
		this.certificateUri = obj.certificateUri;
		if( obj.defaultBrowseNs )
			this.defaultBrowseNs = obj.defaultBrowseNs;
		this.server = obj.server;
	}

	override equals( row:ITargetRow ):boolean{
		let other = row as ServerCnnctn;
		return super.equals(row) && this.url==other.url && this.certificateUri==other.certificateUri;
	}

	override mutation( original:ServerCnnctn ):Mutation[]{
		console.assert( this.canSave );
		let args = super.mutationArgs( original );
		if( this.url!=original?.url )
			args["url"] = this.url;
		if( this.certificateUri!=original?.certificateUri )
			args["certificateUri"] = this.certificateUri;
		return Object.keys( args ).length ? [new Mutation(this.type, this.id, args, original?.id ? MutationType.Update : MutationType.Create)] : [];
	}

	getNs( segment:string ):number{
		let browse = toBrowse( segment, this.defaultBrowseNs );
		return browse.ns;
	}
	removeNs( segment:string ):string{ return toBrowse( segment, this.defaultBrowseNs ).name.toString(); }

	get properties():ServerCnnctn{ let properties = new ServerCnnctn(this); return properties; }
	url:string;
	certificateUri:string;
	defaultBrowseNs:number=1;
	server:Server;
	static typeName = "ServerConnection";
}
export type ServerCnnctnProps = TargetRowProps & { url:string; certificateUri:string; defaultBrowseNs?:number; server:Server };