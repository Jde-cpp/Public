import { Mutation } from '../model/ql/Mutation';
import { Field, FieldKind } from '../model/ql/schema/Field';
import { MutationSchema } from '../model/ql/schema/MutationSchema';
import { TableSchema } from '../model/ql/schema/TableSchema';

export type TypeName = string;
export type Log = (m:string)=>void;
export type Query = {text:string, vars:any};

export interface IGraphQL{
	ql<T>( q:Query, log:Log ):Promise<T>;
	query<T>( ql: string, vars:any, log:Log ):Promise<T>;
	querySingle<T>( ql: string, vars:any, log:Log ):Promise<T>;
	schema( names:string[], log:Log ):Promise<TableSchema[]>;
	schemaWithEnums( type:string, log:Log ):Promise<TableSchema>;
	mutate<T>( ql: string|Mutation|Mutation[], log:Log ):Promise<T>;
	mutations( log:Log ):Promise<MutationSchema[]>;

	targetQuery( schema:TableSchema, target: string, showDeleted:boolean, excludedColumns:string[] ):string;
	subQueries( typeName: string, id: number ):string[];
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