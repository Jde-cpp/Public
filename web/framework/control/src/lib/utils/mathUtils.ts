import { std } from './stl';

export class StatResult
{
	average:number=0.0;
	variance:number=0.0
	min:number=0.0
	max:number=0.0
};
//export default class Jde
//{
	export function sum<T>( values: T[], fnctn: (element:T) => number ):number
	{
		return std.accumulate( values, 0, (sum,value)=>sum+fnctn(value) );
	}
//}
export class MathUtilities
{
	static Statistics( values:number[], calcVariance:boolean=true ):StatResult
	{
		if( values.length==0 )
			return new StatResult();

		var size = values.length;
		let sum:number=0, min=Number.MAX_VALUE, max=Number.MIN_VALUE,variance:number;
		for( let value of values )
		{
			sum += value;
			min = Math.min( min, value );
			max = Math.max( max, value );
		}
		const average = sum/size;
		if( size>1 && calcVariance )
		{
			let varianceFunction = ( accumulator:number, val:number )=>
			{
				const diff = val - average;
				return accumulator + diff*diff / (size - 1);//sample?
			};
			variance = std.accumulate( values, 0.0, varianceFunction );
		}
		return { average:average, variance:variance, min:min, max:max };
	}
}