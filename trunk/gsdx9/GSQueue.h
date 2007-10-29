#pragma once

template<class T> class GSQueue : public CCritSec
{
	CAtlList<T> _queue;
	CSemaphore _put;
	CSemaphore _get;
	CAMEvent _enqueue;
	CAMEvent _dequeue;

public:
	GSQueue(LONG count) 
		: _put(count, count)
		, _get(0, count)
		, _enqueue(TRUE)
		, _dequeue(TRUE)
	{
		_dequeue.Set();
	}

	virtual ~GSQueue()
	{

	}

	size_t GetCount()
	{
		CAutoLock cAutoLock(this);

		return _queue.GetCount();
	}

	CAMEvent& GetEnqueueEvent()
	{
		return _enqueue;
	}

	CAMEvent& GetDequeueEvent()
	{
		return _dequeue;
	}

	void Enqueue(T item)
	{
		_put.Lock();

		{
			CAutoLock cAutoLock(this);

			_queue.AddTail(item);

			_enqueue.Set();
			_dequeue.Reset();
		}

		_get.Unlock();
	}

	T Dequeue()
	{
		T item;

		_get.Lock();

		{
			CAutoLock cAutoLock(this);

			item = _queue.RemoveHead();

			if(_queue.IsEmpty())
			{
				_enqueue.Reset();
				_dequeue.Set();
			}
		}

		_put.Unlock();

		return item;
	}
};
