import { assert, getEnumName } from '../../utilities/utils';
import { TypeName } from '../../services/IGraphQL';
import { StringUtils } from '../../utilities/StringUtils';

export enum MutationType{
		Create,
		Update,
		Delete,
		Purge,
		Add,
		Remove
}
export class Mutation{
	constructor( private _typeName:TypeName=null, private id:number=0, public args:any=null, private type:MutationType=null, private result:string[]=null ){
	}
	add( child:Mutation ){
		if( !this.#children.has(child.typeName) )
			this.#children.set( child.typeName, [child] );
		else
			this.#children.get(child.typeName).push( child );
	}
	toString():string|null{
		let query = null;
		let addRemoveQuery = ()=>{
			let addRemove = '';
			for( let [key, keyItems] of this.#children ){
				for( let item of keyItems )
					addRemove += item.toString();
			}
			return addRemove;
		};
		if( this.type==MutationType.Remove ){
			query = `remove${StringUtils.capitalize(this.typeName)}`;
			if( this.id )
				query += `( id:${this.id} )`;
			if( this.#children.size )
				query += addRemoveQuery();
		}
		if( this.args && Object.keys(this.args).length ){
			let args = this.args;
			if( this.type==MutationType.Create && this.#children.size ){
				args = JSON.parse(JSON.stringify(this.args));
				for( let [key, keyItems] of this.#children ){
					args[key] = [];
					for( let item of keyItems )
						args[key].push( item.args );
				}
			}
			query = `${getEnumName(MutationType,this.type).toLowerCase()}${StringUtils.capitalize(this.typeName)}( `;
			if( this.id )
				query += `id:${this.id}`;
			let argClause = ( itemArgs:any, continuation:boolean=true )=>{
				let clause = "";
				for( let key in itemArgs ){
					let value = itemArgs[key];
					if( continuation )
						clause+=", ";
					else
						continuation = true;
					if( value===null )
						clause += `${key}:null`;
					else if( typeof value == "string" )
						clause += `${key}:"${value}"`;
					else if( typeof value == "number" || typeof value=="boolean" )
						clause += `${key}:${value}`;
					else if( Array.isArray(value) )
						clause += `${key}:[${value.join(",")}]`;
					else{
						assert( typeof value == "object" );
						clause += `${key}:{${argClause(value, false)}}`;
					}
				}
				return clause;
			}
			query+=argClause( args, this.id>0 );
			query += ` )`;
			if( this.result?.length )
				query += `{${this.result.join(",")}}`;
			if( this.type!=MutationType.Create && this.#children.size ){
				query += addRemoveQuery();
			}
		}
		else if( this.#children.size ){
			query = '';
			for( let [key, keyItems] of this.#children ){
				for( let item of keyItems )
					query += item.toString();
			}
		}
		return query;
	}

	#children:Map<TypeName,Mutation[]> = new Map<TypeName,Mutation[]>();
	get isEmpty():boolean{ return this.type!=MutationType.Remove && !this.args && !this.#children.size; }
	get typeName(){ return this._typeName; }
}