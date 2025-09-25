import { MetaObject } from "./MetaObject";
import { Field, FieldKind } from "./Field";
import { IEnum } from '../../../services/IGraphQL';

export class TableSchema extends MetaObject{
	constructor( j ){
		super( j.name );
		j.fields.forEach( (x)=>this.fields.push( new Field(x) ) );
	}
	get columns():string{
		var result = '';
		for( const column of this.nonListFields )
			result += column.type.underlyingKind==FieldKind.OBJECT ? `${column.type.underlyingVariableName}{ id name } ` : `${column.name} `;

		return result;
	}
	fields = new Array<Field>();
	get nonListFields():Array<Field>{ return this.fields.filter((x)=>x.type.kind!=FieldKind.LIST); }
	get listFields():Array<Field>{ return this.fields.filter((x)=>x.type.kind==FieldKind.LIST); }
	get display(){ return this.subType?.collectionDisplay ?? this.type+'s'; }
	subType:MetaObject;
	enums: Map<string, IEnum[]>;
}
