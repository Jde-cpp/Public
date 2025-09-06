import { QLSchema } from './Schema';
//https://graphql.org/learn/introspection/
export enum FieldKind{
	SCALAR=0,
	OBJECT=1,
	INTERFACE=2,
	UNION=3,
	ENUM=4,
	INPUT_OBJECT=5,
	LIST=6,
	NON_NULL=7
}

export class OfType extends QLSchema{
	constructor( j ){
		super( j );
		this.kind = typeof j.kind==="string" ? FieldKind[j.kind] : j.kind;
	}
	kind:FieldKind;
}

export class FieldType extends OfType{
	constructor( j ){
		super( j )
		this.ofType = j.ofType ? new OfType( j.ofType ) : null;
	}
	get underlyingKind():FieldKind{ return this.ofType?.kind ?? this.kind; }
	get underlyingName():string{ return this.ofType?.name ?? this.name; }
	get underlyingVariableName():string{ return this.underlyingName.charAt(0).toLowerCase()+this.underlyingName.slice(1) ; }
	ofType:OfType;
}

export class Field extends QLSchema{
	constructor( j ){
		super( j )
		this.type = j.type ? new FieldType( j.type ) : undefined;
	}
	static filter( fields:Field[], excludedColumns:string[], includeDeleted:boolean ):Field[]{
		return fields.filter( (x)=>
			(x.displayed || x.name=="id")
		&& !excludedColumns?.includes(x.name)
		&& (includeDeleted || x.name!="deleted") );
	}
	static filterSort( fields:Field[], order:string[], excludedColumns:string[]=[], includeDeleted:boolean=false ):Field[]{
		const sort = ( x:Field,y:Field )=>{
			const yIndex = order.indexOf( y.name )+1;
			const xIndex = order.indexOf( x.name )+1;
			return ( xIndex || order.length )-( yIndex || order.length );
		};
		return this.filter( fields, excludedColumns, includeDeleted ).sort( sort );
	}
	get displayed():boolean{ return this.#displayed ?? (this.type.ofType?.name!="ID" && this.name!="attributes" && this.type.kind!=FieldKind.LIST); } set displayed(x){this.#displayed=x;} #displayed:boolean;
	type:FieldType;
}
