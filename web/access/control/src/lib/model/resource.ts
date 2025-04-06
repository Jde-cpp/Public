import { Permission, Rights } from "./Permission";
import { TargetRow } from "jde-framework";

export type ResourcePK=number;

export class Resource extends TargetRow<Resource>{
	static from( resources:Partial<Resource>[] ):Resource[]{
		return resources.map( r=>new Resource(r) );
	}
	constructor(obj:any){
		super("Resource", obj);
		this.schema = obj.schema ?? obj.schemaName;
		this.criteria = obj.criteria;
		this.availableRights = obj.availableRights ?? Permission.toRights( obj.allowed );
	}

	availableRights:Rights;
	criteria:string;
	schema:string;
}
