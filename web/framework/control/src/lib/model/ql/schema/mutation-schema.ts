import { OfType } from './field';
import { QLSchema } from "./ql-schema";

export class Arg{
	name!:string;
	defaultValue!:string;
	type!:OfType;
}

export class MutationSchema extends QLSchema{
	args!:Arg[];
}
