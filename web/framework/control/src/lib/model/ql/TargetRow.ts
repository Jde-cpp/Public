import { Row } from './Row';
import { Mutation, MutationType } from './Mutation';
import { assert, clone } from '../../utilities/utils';

export type Target = string;
export abstract class ITargetRow extends Row{
	constructor(type:string,obj:any){
		super( type );
		if( typeof obj=="number" )
			this.id = obj;
		else{
			this.id = obj.id;
			this.target = obj.target;
			this.name = obj.name;
			this.created = obj.created ? new Date(obj.created) : undefined;
			this.updated = obj.updated ? new Date(obj.updated) : undefined;
			this.deleted = obj.deleted ? new Date(obj.deleted) : undefined;
			this.description = obj.description;
		}
	}

	static idArray( from:ITargetRow[] ):any{
		let clone = [];
		for( let item of from ?? [] )
			clone.push( item.id );
		return clone;
	}

	override equals( row:ITargetRow ):boolean{
		return this.target==row.target && this.name==row.name && this.description==row.description;
	}

	protected addRemoveMutations<T extends TargetRow<T>>( parentType:string, originalChildren:TargetRow<T>[], modifiedChildren:TargetRow<T>[], input:any ):Mutation[]{
		let y = [];
		let getMutations = ( changes:TargetRow<T>[], type:MutationType )=>{
			for( let change of changes )
				y.push( new Mutation(parentType, change.id, input, type) );
		}
		getMutations( TargetRow.notSubset(originalChildren, modifiedChildren), MutationType.Remove );
		getMutations( TargetRow.notSubset(modifiedChildren, originalChildren), MutationType.Add );

		return y;
	}

	protected childMutations( parent:ITargetRow, originalChildren:ITargetRow[], modifiedChildren:ITargetRow[], input:any={} ):Mutation[]{
		let y = [];
		let addMutations = ( changes:number[], type:MutationType )=>{
			if( !changes.length )
				return;
			let mutationInput = clone( input );
			let keys = Object.keys(input);
			let inputObj = keys.length ? mutationInput[Object.keys(mutationInput)[0]] : mutationInput; // ? role:id : id:
			inputObj.id = changes;
			y.push( new Mutation(parent.type, parent.id, mutationInput, type) );
		}
		let original = originalChildren.map( x=>x.id );
		let current = modifiedChildren.map( x=>x.id );
		addMutations( original.filter((x)=>!current.includes(x)), MutationType.Remove );
		addMutations( current.filter((x)=>!original.includes(x)), MutationType.Add );

		return y;
	}

	get canSave():boolean{ return this.name?.length>0 && this.target?.length>0; }

	readonly id:number;
	target:Target;
	name:string;
	readonly created:Date;
	readonly updated:Date;
	readonly deleted:Date;
	description:string;
}

export abstract class TargetRow<T extends TargetRow<T>> extends ITargetRow{
	constructor(type:string,obj:any){
		super(type, obj);
	}

	mutationArgs( original:T ){
		let args = {};
		if( this.target!=original?.target )
			args["target"] = this.target;
		if( this.name!=original?.name )
			args["name"] = this.name;
		if( this.description!=original?.description )
			args["description"] = this.description;
		return args;
	}
	mutation( original:T ):Mutation[]{
		assert( this.canSave );
		let args = this.mutationArgs( original );
		if( this.target!=original?.target )
			args["target"] = this.target;
		if( this.name!=original?.name )
			args["name"] = this.name;
		if( this.description!=original?.description )
			args["description"] = this.description;
		return Object.keys(args).length ? [new Mutation( this.type, this.id, args, original.id ? MutationType.Update : MutationType.Create )] : [];
	}

	static notSubset<T extends TargetRow<T>>( a:T[], b:T[] ):T[]{
		let y = [];
		for( let item of a )
			if( !b.find( x=>x.id==item.id ) )
				y.push( item );
		return y;
	}
}