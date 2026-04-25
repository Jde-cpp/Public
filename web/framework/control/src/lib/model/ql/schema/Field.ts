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
	type:FieldType;
	get underlyingKind(){ return this.type.kind; }
	get isNumber(){ return this.type.underlyingKind==FieldKind.SCALAR && ["Int", "Float", "ID", "UInt"].includes(this.type.underlyingName); }
	get isString(){ return this.type.underlyingKind==FieldKind.SCALAR && this.type.underlyingName=="String"; }
	get isBoolean(){ return this.type.underlyingKind==FieldKind.SCALAR && this.type.underlyingName=="Boolean"; }
	get isEnum(){ return this.type.underlyingKind==FieldKind.ENUM; }
	get isDateTime(){ return this.type.underlyingKind==FieldKind.SCALAR && ["DateTime"].includes(this.type.underlyingName); }
	get isNullable(){ return this.type.kind!=FieldKind.NON_NULL; }
}
