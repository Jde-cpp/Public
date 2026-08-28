import { EnumValue, Type } from "jde-framework";
import { NodeId } from "./node-id";

export class Enum{
	constructor( public id:NodeId, value:Type ){
		this.name = value.name;
		this.enumValues = value.enumValues!;
	}
	name:string;//the DataType node's display name, e.g. DeviceHealthEnumeration.
	enumValues:EnumValue[];
}