import { EnumValue, Type } from "jde-framework";
import { NodeId } from "./node-id";

export class Enum{
	constructor( public id:NodeId, value:Type ){
		this.enumValues = value.enumValues!;
	}
	enumValues:EnumValue[];
}