import { KeyValue } from '@angular/common';
import { Pipe, PipeTransform } from '@angular/core';

@Pipe({name: 'enumKeys'})
export class EnumKeysPipe implements PipeTransform {
	transform(value: any): KeyValue<number,string>[] {
		let y = new Array<KeyValue<number,string>>();
		for( const key in value ){
			if( !isNaN(Number(key)) )
				y.push( {key:Number(key), value:value[key]} );
		}
		return y;
	}
}