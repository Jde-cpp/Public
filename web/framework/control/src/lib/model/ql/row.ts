import { MetaObject } from "./schema/meta-object";

export abstract class Row extends MetaObject{
	constructor( type:string ){ super(type); }

	equals( row:Row ):boolean{
		return JSON.stringify(this)==JSON.stringify(row);
	}
}
