export type Day = number;
export type Minutes = number;

export class DateUtils{
	static endOfDay( value:Date|null=null ):Date{
		let copy = value ? new Date(value) : new Date();
		copy.setUTCHours( 23, 59, 59, 0 );
		return copy;
	}
	static beginningOfDay( value:Date|null=null ){
		let copy = value ? new Date( value ) : new Date();
		copy.setUTCHours( 0, 0, 0, 0 );
		return copy;
	}
	//The inverse of toDays, which floors UTC epoch-days - so this is UTC midnight of `value`, and
	//`toDays(fromDays(d))==d` everywhere.  It used to build its epoch from LOCAL 1970-01-01 midnight, which made the pair
	//disagree by the zone offset:  east of UTC every round trip lost a day (`toDays(fromDays(d))==d-1`), and DateRange.toDate,
	//which adds the offset back to render a picker date, applied it twice and showed 22:00 of the day before.
	//endOfDay is the last whole second of the day, as before.
	static fromDays( value:Day, endOfDay=false ):Date{
		return new Date( endOfDay ? (value+1)*86400000-1000 : value*86400000 );
	}
	static toDays( value:Date ):Day{
		if( value==null )
			console.error( "toDays( null )" );
		return Math.floor( value.getTime()/(24*60*60000) );
	}
	static dayOfWeek( date:Date ){
		const days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
		return days[ date.getUTCDay() ];
	}
	static dayOfWeekAbr( date:Date ){
		const days = ["Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"];
		return days[ date.getUTCDay() ];
	}
	//UTC fields throughout, like dayOfWeekAbr below and everything else here:  fromDays now hands back UTC midnight, so the
	//LOCAL getters this used to mix in named the day before anywhere west of UTC.
	static display( date:Date ):string{
		const now = new Date();
		const showYear = date.getUTCFullYear()<now.getUTCFullYear() && ( now.getUTCFullYear()-date.getUTCFullYear()>1 || date.getUTCMonth()<=now.getUTCMonth() );
		const showMonth = showYear || date>now || DateUtils.toDays(now)-DateUtils.toDays(date)>6;
		let display = "";
		if( showYear )
			display = `${date.getUTCFullYear()-2000}-`;
		if( showMonth )
			display += `${date.getUTCMonth()+1}-${date.getUTCDate()}`;
		else
			display = this.dayOfWeekAbr( date );

		return display;
	}
	static displayDay( day:Day ){
		return DateUtils.display( DateUtils.fromDays(day) );
	}
	static asUtc( date:Date ):Date{
		const y = new Date( date.getTime() + date.getTimezoneOffset()*60000 );
		return y;
	}

	static isMidnight( date:Date ):boolean{
		return date.getUTCHours()==0 && date.getUTCMinutes()==0 && date.getUTCSeconds()==0 && date.getUTCMilliseconds()==0;
	}
}