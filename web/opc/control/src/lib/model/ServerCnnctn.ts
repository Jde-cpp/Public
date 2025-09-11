import { cloneClassArray, ITargetRow, Mutation, MutationType, TargetRow } from "jde-framework";

export type CnnctnPK = number;
export type CnnctnTarget = string;
export class ServerCnnctn extends TargetRow<ServerCnnctn>{
	constructor( obj:any ){
		super(ServerCnnctn.typeName, obj);
		this.url = obj.url;
		this.certificateUri = obj.certificateUri;
		if( obj.defaultBrowseNs )
			this.defaultBrowseNs = obj.defaultBrowseNs;
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
		const index = segment.indexOf( "~" );
		if( index>0 ){
			const nsStr = segment.substring( 0, index );
			if( /^-?\d+$/.test(nsStr) )
				return +nsStr;
		}
		return this.defaultBrowseNs;
	}
	removeNs( segment:string ):string{
		const index = segment.indexOf( "~" );
		if( index>0 ){
			const nsStr = segment.substring( 0, index );
			if( /^-?\d+$/.test(nsStr) )
				return segment.substring( index + 1 );
		}
		return segment;
	}

	get properties():ServerCnnctn{ let properties = new ServerCnnctn(this); return properties; }
	url:string;
	certificateUri:string;
	defaultBrowseNs:number=1;
	static typeName = "ServerConnection";
}