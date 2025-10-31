import { Subject,Observable,NextObserver,CompletionObserver,Subscription, PartialObserver, Subscriber } from 'rxjs';

//https://stackoverflow.com/questions/46151594/how-to-handle-progress-update-using-reactivex-observables-subjects
export interface IProgressObserver<T> extends NextObserver<T>
{
	progress:( pct:number )=>void;
}

export class ProgressObservable<T> extends Observable<T>
{
	subscribe2( observer: IProgressObserver<T> ): Subscription
	{
		return this.subscribe( observer );
	}
}

export class ProgressSubject<T> extends Subject<T> implements IProgressObserver<T>
{
	subscribe2( observer?: IProgressObserver<T> ): Subscription
	{
		this._observers.push( observer );
		return this.subscribe( observer );
	}
	progress( pct:number ){ this._observers.forEach(observer=>{observer.progress(pct);}) };
	private _observers:IProgressObserver<T>[]=[];
}