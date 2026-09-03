import { DateUtils } from './date-utils';
import { DateRange, DateRangeSettings, TimeFrame } from '../shared/date-range/date-range';

//angular-review3 #15: fromDays built its epoch from LOCAL 1970-01-01 while toDays floors UTC epoch-days, so the pair
//disagreed by the zone offset - `toDays(fromDays(d))==d-1` everywhere east of UTC.
//The suite runs in whatever zone the machine is in, so the offset is faked to cover both hemispheres.  It reaches only the
//code that ASKS for the offset (DateRange.toDate/fromDate, dayCount) - it cannot change how `new Date(y,m,d)` resolves - so
//the DateUtils tests below pin the absolute instant instead, which is the zone-free contract.
const withOffset = ( minutes:number, body:()=>void )=>{
	const original = Date.prototype.getTimezoneOffset;
	Date.prototype.getTimezoneOffset = function(){ return minutes; };//negative = east of UTC, as the platform reports it
	try{ body(); }
	finally{ Date.prototype.getTimezoneOffset = original; }
};
const zones = [ {name: "UTC+2 (Berlin)", offset: -120}, {name: "UTC", offset: 0}, {name: "UTC-5 (New York)", offset: 300}, {name: "UTC+13 (Auckland, DST)", offset: -780} ];
const days = [ 0, 1, 19000, 20000, 20323 ];

describe( 'DateUtils day round trip', ()=>{
	it( 'fromDays is exactly UTC midnight of the day - the instant toDays floors to', ()=>{
		for( const d of days ){
			expect( DateUtils.fromDays(d).getTime() ).toBe( d*86400000 );
			expect( DateUtils.isMidnight(DateUtils.fromDays(d)) ).toBe( true );
		}
	} );

	it( 'toDays is the inverse of fromDays', ()=>{
		for( const d of days )
			expect( DateUtils.toDays(DateUtils.fromDays(d)) ).toBe( d );
	} );

	it( 'endOfDay is the last whole second of the same day', ()=>{
		const end = DateUtils.fromDays( 20000, true );
		expect( DateUtils.toDays(end) ).toBe( 20000 );
		expect( end.getTime()-DateUtils.fromDays(20000).getTime() ).toBe( 24*60*60*1000-1000 );
	} );

	it( 'displayDay names the day it was given, not the one before', ()=>withOffset( 300, ()=>{
		expect( DateUtils.displayDay(DateUtils.toDays(new Date(Date.UTC(2024, 2, 15)))) ).toContain( "3-15" );
	} ) );
} );

describe( 'DateRange picker conversions', ()=>{
	for( const zone of zones ){
		//toDate renders a day as the picker wants it - LOCAL midnight - and fromDate reads that back.
		it( `toDate/fromDate round trip in ${zone.name}`, ()=>withOffset( zone.offset, ()=>{
			for( const d of days )
				expect( DateRange.fromDate(DateRange.toDate(d)) ).toBe( d );
		} ) );

		it( `toDate hands the picker local midnight in ${zone.name}`, ()=>withOffset( zone.offset, ()=>{
			const picked = DateRange.toDate( 20000 );
			//the platform's real local getters cannot be faked, so check the instant instead:  local midnight is UTC midnight
			//shifted by the offset the picker will apply.
			expect( picked.getTime() ).toBe( 20000*86400000+zone.offset*60000 );
		} ) );
	}

	//dayCount read getUTC* fields off max and rebuilt them as a LOCAL midnight, so toDays floored to the previous day east of
	//UTC and the count came out one too big - and differed by zone.
	it( 'a month dayCount is the real span, the same in every zone', ()=>{
		const march15 = DateUtils.toDays( new Date(Date.UTC(2024, 2, 15)) );
		const counts = zones.map( z=>{ let y = 0; withOffset( z.offset, ()=>{ y = new DateRangeSettings(TimeFrame.Month, march15).dayCount!; } ); return y; } );
		expect( counts ).toEqual( zones.map(()=>29) );//2024-02-15 → 2024-03-15, a leap February
	} );
} );
