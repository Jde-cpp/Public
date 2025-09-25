import { StringUtils } from '../../../utilities/StringUtils';

export class MetaObject{
	constructor( public type:string ){} //User
	get collectionDisplay(){ return this.type+'s'; }
	get collectionName(){ return MetaObject.toCollectionName(this.type); }
	get singular(){ return this.type.charAt(0).toLowerCase() + this.type.slice(1); }
	get idReferenceName(){ return this.singular+"Id"; }
	static toTypeFromCollection( collectionName:string ){ return StringUtils.singular(StringUtils.capitalize(collectionName[0]) + collectionName.slice(1)); }
	static toCollectionName(type:string):string{ return type.charAt(0).toLowerCase() + type.slice(1) + 's'; }
}