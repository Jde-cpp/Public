import { Mutation, MutationType, SlugRow } from "jde-framework";
import { Role } from "./role";

export class Acl {
	static roleMutations( identityId:number, original:Role[], modified:Role[] ):Mutation[]{
		let y = new Array<Mutation>;
		let getMutations = ( changes:Role[], type:MutationType )=>{
			for( let change of changes )
				y.push( new Mutation(Acl.typeName, undefined, { identity:{id:identityId}, role:{id:change.id} }, type) );
		}

		getMutations( SlugRow.notSubset(original, modified), MutationType.Purge );
		getMutations( SlugRow.notSubset(modified, original), MutationType.Create );

		return y;
	}
	static typeName = "Acl";
}