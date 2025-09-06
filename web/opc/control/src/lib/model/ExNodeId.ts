import { NodeKey, INodeId, NodeId, NodeIdJson } from "./NodeId";

export interface IExNodeId extends INodeId{
	serverIndex:number;
	nsu:string;
}
export type ExNodeIdJson = {nsu?:string,serverIndex?:number} & NodeIdJson;

export class ExNodeId extends NodeId /*implements IExNodeId*/{
	constructor( json: ExNodeIdJson ){
		super(json);
		this.serverIndex = json.serverIndex;
		this.nsu = json.nsu;
		if( !this.ns && !this.nsu && ExNodeId.defaultNS )
			this.ns = ExNodeId.defaultNS;
	}
	public override equals(obj: ExNodeId) : boolean {
		return super.equals(obj) && (this.serverIndex ?? 0)==(obj.serverIndex ?? 0) && (this.nsu ?? 0)==(obj.nsu ?? 0);
	}

	override get key():NodeKey{
		if( !this._key ){
			let j = this.toJson();
			if( this.serverIndex )
				j["serverIndex"] = this.serverIndex;
			this._key = Symbol.for( JSON.stringify(j) );
		}
		return this._key;
  }
	serverIndex:number;
	nsu:string;
//	#key:NodeKey; //for Map
	static defaultNS:number = 0;
}
