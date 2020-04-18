
#include <memory>      // for std::shared_ptr
#include <functional>
#include <queue>       // for std::queue
#include <thread>      // 
#include <mutex>
#include <condition_variable>
 

template <class T>
class FutureImpl {
public:
	FutureImpl(): _ready(false), _success(false) {}

	~FutureImpl() {}

	bool get(T* value) {
		std::unique_lock<std::mutex> lock(_mutex);
		while (!_ready)
			_cond.wait(lock);

		bool r = _success;
		if (value != NULL)
			*value = _value;
		return r;
	}

	void set(bool success, const T& value) {
		std::unique_lock<std::mutex> lock(_mutex);
		_success = success;
		_value = value;
		_ready = true;
		_cond.notify_all();
	}

private:
	std::mutex _mutex;
	std::condition_variable _cond;
	T _value;
	bool _ready;
	bool _success;
};


template <class T>
class Future {
public:
	Future(): _impl(new FutureImpl<T>()) {}

	bool get(T *value) {
		return _impl->get(value);
	}

	void set(bool success, const T&& value) {
		_impl->set(success, value);
	}

private:
	std::shared_ptr<FutureImpl<T>> _impl;
};


class Task {
public:
	virtual void run() = 0;
	virtual void cancel() = 0;
};

template<class R>
class TaskImpl : public Task {
	std::function<R()> _fn;
	Future<R> _future;
public:
	explicit TaskImpl(std::function<R()>&& fn): _fn(fn) {}

	void run() {
		try {
			_future.set(true, _fn());
		} catch(...) {
			_future.set(false, R());
		}
	}

	void cancel() {
		_future.set(false, R());
	}

	Future<R> get_future() {
		return _future;
	}
};


/* A pthread based thread pool */

class ThreadPool {
private:
	// TaskQueue: a blocking queue implementation w/o limit on its size.
	// 
	// 'add_task' enqueues tasks to the queue
	// 'next_task' dequeues tasks from the queue
	//
	// A thread trying to dequeue from an empty queue will be blocked 
	// (using pthread_cond_wait) until some other thread submits a task 
	// to the queue.
	//
	// Closing a TaskQueue will clear all tasks in the queue. If a queue is 
	// closed, 'add_task' does nothing and 'next_task' returns NULL.
	// 
	// Thread safety: after a TaskQueue is initialized (or constructed), 
	// it is thread-safe.
	//
	class TaskQueue {	
	public:
		TaskQueue(): _closed(false) {}
		
		Task *next_task() {
			Task *next = NULL;
			std::unique_lock<std::mutex> lock(_mutex);
			
			while (!_closed && _tasks.empty())
				_cond.wait(lock);
			
			// if we are here, the queue is closed 
			// or it is not empty.

			if (_closed) {
				return NULL;
			}

			next = _tasks.front();
			_tasks.pop();
			return next;
		}
		
		bool add_task(Task *task) {
			std::unique_lock<std::mutex> lock(_mutex);
			if (!_closed) {
				_tasks.push(task);
				_cond.notify_one();
			} else {
				task->cancel();
				delete task;
			}
			return !_closed;
		}

		void close() {
			// take notice of the "lost wake-up" problem
			std::unique_lock<std::mutex> lock(_mutex);
			
			// clear tasks in the queue
			while (!_tasks.empty()) {
				Task *task = _tasks.front();
				_tasks.pop();
				task->cancel();
				delete task;
			}
			
			// set the queue closed and wake up all threads 
			// waiting on '_cond'
			_closed = true;
			_cond.notify_all();
		}

		bool closed() {
			return _closed;
		}
	
	private:
		std::queue<Task*> _tasks; // the actural task queue
		std::mutex _mutex;
		std::condition_variable _cond;
		volatile bool _closed;    // TODO: is 'volatile' necessary here?
	};
	
	
	static void run_tasks(TaskQueue& task_queue) {
		while (!task_queue.closed()) { // test if the pool is shutdown
			// look for a task to execute
			Task *next = task_queue.next_task();
			if (next != NULL) { 
				next->run(); 
				delete next; 
			}
		}
	}

public:	
	explicit ThreadPool(int nthreads): _nthreads(nthreads) {
		for (int i = 0; i < nthreads; ++i) {
			_threads.emplace_back([this](){
					run_tasks(this->_task_queue);
				});
		}
	}

	// Submit a task to the thread pool. if the pool is shutdown, noop
	//    usage: submit(new Task(...))
	//    returns: true on success, false on fail
	template <class Fn, class... Args>
	auto async_apply(Fn&& fn, Args&&... args) 
		-> Future<typename std::result_of<Fn(Args...)>::type> {
 		using R = typename std::result_of<Fn(Args...)>::type;
    		auto f = std::bind(std::forward<Fn>(fn), 
				std::forward<Args>(args)...);
		TaskImpl<R>* task = new TaskImpl<R>(f);
		_task_queue.add_task(task);
		return task->get_future();
	}

	// Shutdown the thread pool. after the pool is shutdown, it can't
	// accept any tasks and no tasks will be executed except the running 
	// ones.
	void close() {
		_task_queue.close();
	}

	bool closed() {
		return _task_queue.closed();
	}
	
	// Wait all threads to terminate. 
	void join() {
		for (int i = 0; i < _nthreads; ++i)
			_threads[i].join();
	}

private:
	std::vector<std::thread> _threads;      
	int _nthreads;
	TaskQueue _task_queue;
};

