import { Mutation, MutationType, TargetRow } from "jde-framework";
import { Role } from "./Role";

export class Acl {
	static roleMutations( identityId:number, original:Role[], modified:Role[] ):Mutation[]{
		let y = new Array<Mutation>;
		let getMutations = ( changes:Role[], type:MutationType )=>{
			for( let change of changes )
				y.push( new Mutation(Acl.typeName, undefined, { identity:{id:identityId}, role:{id:change.id} }, type) );
		}

		getMutations( TargetRow.notSubset(original, modified), MutationType.Purge );
		getMutations( TargetRow.notSubset(modified, original), MutationType.Create );

		return y;
	}
	static typeName = "Acl";
}