
export abstract class IProfileStore{
	// defaultValue to null out setting if==default
	abstract load<T>( key:string, defaultValue:T ):Promise<T>;
	abstract save<T>( name:string, value:T|string|null ):void;
}