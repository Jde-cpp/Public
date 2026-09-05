import { InjectionToken } from '@angular/core';

export interface IEnvironment
{
	get<T>( key:string ): T;
}
//angular-review3 C13: a typed token in place of the string one - a typo now fails the build instead of resolving to nothing at runtime, and inject() can take it.
export const IENVIRONMENT = new InjectionToken<IEnvironment>( 'IEnvironment' );
