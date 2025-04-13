import { Mutation, MutationType, TargetRow } from "jde-framework";
import { Role } from "./Role";

export class Acl {

	//createAcl( identity:{{ id:{} }}, role:{{id:{}}} )
	static roleMutations( identityId:number, original:Role[], modified:Role[] ):Mutation[]{
		let y = [];
		let getMutations = ( changes:Role[], type:MutationType )=>{
			for( let change of changes )
				y.push( new Mutation(Acl.typeName, null, { identity:{id:identityId}, role:{id:change.id} }, type) );
		}

		getMutations( TargetRow.notSubset(original, modified), MutationType.Purge );
		getMutations( TargetRow.notSubset(modified, original), MutationType.Create );

		return y;
	}
	static typeName = "Acl";
}