import { MetaObject } from "./schema/MetaObject";

export abstract class Row extends MetaObject{
	constructor( type:string ){ super(type); }

	equals( row:Row ):boolean{
		return JSON.stringify(this)==JSON.stringify(row);
	}
}
