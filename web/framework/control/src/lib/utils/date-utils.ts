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
	// //date control sends a local time date, but
	static fromDays( value:Day, endOfDay=false ):Date{
		let t = new Date( 1970, 0, 1 ), offset = 0;
		if( endOfDay ){
			++value;
			--offset;
		}
		t.setSeconds( value*24*60*60+offset );
		return t;
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
	static display( date:Date ):string{
		const now = new Date();
		const showYear = date.getFullYear()<now.getFullYear() && ( now.getFullYear()-date.getFullYear()>1 || date.getMonth()<=now.getMonth() );
		const showMonth = showYear || date>now || DateUtils.toDays(now)-DateUtils.toDays(date)>6;
		let display = "";
		if( showYear )
			display = `${date.getFullYear()-2000}-`;
		if( showMonth )
			display += `${date.getMonth()+1}-${date.getDate()}`;
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