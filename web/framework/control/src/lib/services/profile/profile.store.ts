
export interface IProfileStore{
	load<T>( key:string, defaultValue:T ):Promise<T>;
	save<T>( name:string, value:T|string|null ):void;
}