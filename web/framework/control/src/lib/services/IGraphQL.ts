import { Mutation } from '../model/ql/Mutation';
import { Field, FieldKind } from '../model/ql/schema/Field';
import { MutationSchema } from '../model/ql/schema/MutationSchema';
import { TableSchema } from '../model/ql/schema/TableSchema';

export type TypeName = string;
export type Log = (m:string)=>void;

export interface IGraphQL{
	query<T>( ql: string ):Promise<T>;
	querySingle<T>( ql: string ):Promise<T>;
	schema( names:string[] ):Promise<TableSchema[]>;
	schemaWithEnums( type:string, log?:Log ):Promise<TableSchema>;
	mutation<T>( ql: string|Mutation|Mutation[], log?:Log ):Promise<T>;
	mutations():Promise<MutationSchema[]>;

	targetQuery( schema:TableSchema, target: string, showDeleted:boolean ):string;
	subQueries( typeName: string, id: number ):string[];
	excludedColumns( tableName:string ):string[];
	toCollectionName( collectionDisplay:string ):string;
}

export type EnumValue = {
	id?:number;
	name:string;
	description?:string;
	isDeprecated?:boolean;
	deprecationReason?:string;
}
export type Type = {
	name:string;
	kind:FieldKind;
	description?:string;
	fields?:Field[];
	enumValues?:EnumValue[];
}

export interface IQueryResult<T>{
	__type:Array<T>;
}