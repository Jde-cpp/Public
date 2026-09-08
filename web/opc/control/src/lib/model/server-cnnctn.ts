import { ITargetRow, Mutation, MutationType, PropertyNames, TargetRow, TargetRowProps  } from "jde-framework";
import { toBrowse } from "./types";
import { Server } from "./server";

export type CnnctnPK = number;
export type CnnctnTarget = string;
export class ServerCnnctn extends TargetRow<ServerCnnctn>{
	constructor( obj:ServerCnnctnProps ){
		super(ServerCnnctn.typeName, obj);
		this.url = obj.url;
		this.certificateUri = obj.certificateUri;
		this.defaultBrowseNs = ServerCnnctn.toNs( obj.defaultBrowseNs );
		this.server = obj.server;
		this.serverError = obj.serverError;
	}

	override get canSave():boolean{ return super.canSave && this.url?.length>0; }//url is non-null in the gateway meta - the base only knows name/target

	override equals( row:ITargetRow ):boolean{
		let other = row as ServerCnnctn;
		return super.equals(row) && this.url==other.url && this.certificateUri==other.certificateUri && ServerCnnctn.toNs(this.defaultBrowseNs)==ServerCnnctn.toNs(other.defaultBrowseNs);//the edited copy still holds the input's raw string
	}

	override mutation( original:ServerCnnctn ):Mutation[]{
		console.assert( this.canSave );
		let args = super.mutationArgs( original );
		if( this.url!=original?.url )
			args["url"] = this.url;
		if( this.certificateUri!=original?.certificateUri )
			args["certificateUri"] = this.certificateUri;
		if( this.defaultBrowseNs!=original?.defaultBrowseNs )
			args["defaultBrowseNs"] = this.defaultBrowseNs;
		return Object.keys( args ).length ? [new Mutation(this.type, this.id, args, original?.id ? MutationType.Update : MutationType.Create)] : [];
	}

	getNs( segment:string ):number{
		let browse = toBrowse( segment, this.defaultBrowseNs );
		return browse.ns!;
	}
	removeNs( segment:string ):string{ return toBrowse( segment, this.defaultBrowseNs ).name.toString(); }

	//The properties form writes the text input's string straight onto the record, so anything that reads the field goes
	//through here:  the constructor keeps it a number (the mutation must send `defaultBrowseNs:2`, not `"2"`), and a
	//cleared/non-numeric/0 entry falls back to the default rather than being saved as garbage.
	static toNs( value:unknown ):number{ return Number(value) || 1; }
	get properties():ServerCnnctn{ let properties = new ServerCnnctn(this as ServerCnnctnProps); return properties; }
	url:string;
	certificateUri:string;
	defaultBrowseNs:number=1;
	server:Server;
	serverError?:string;//the resolver's connect failure, when `server` is unset for an existing row - the Connection tab shows it
	static typeName = "ServerConnection";
}
export type ServerCnnctnProps = TargetRowProps & { url:string; certificateUri:string; defaultBrowseNs?:number; server:Server; serverError?:string };