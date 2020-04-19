#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <memory>      // for std::shared_ptr
#include <functional>  // for std::function, std::bind
#include <queue>       // for std::queue
#include <thread>      // for std::thread
#include <mutex>       // for std::mutex
#include <condition_variable> // for std::condition_variable


// Futures are used to retrieve execution result of a task
// summitted to a thread pool. Each future returned by
// ThreadPool::apply_async() (see below) is associated with
// a unique task, and vice versa.

template <class T>
class FutureImpl {

	std::mutex mutex_; // lock to protect the following fields
	std::condition_variable cond_; // for efficient waiting
	T value_; // return value of the associated task execution
	bool ready_; // true if execution state of the task is ready
	bool success_; // true if the task is successfully executed
	std::string error_; // reason why the task gets failed

public:
	FutureImpl(): ready_(false), success_(false) {}

	// Get execution state of a task. Returns true if the
	// associated task has been successfully exectuted.
	// It will block until the execution result of the task
	// is ready or the task gets cancelled.
	bool get(T* value) {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!ready_)
			cond_.wait(lock);

		bool r = success_;
		if (value != NULL)
			*value = value_;
		return r;
	}

	std::string error() const {
		return error_;
	}

	// Set execution state of a task.
	void set(bool success, const T& value, 
			const std::string& error) {
		std::unique_lock<std::mutex> lock(mutex_);
		success_ = success;
		value_ = value;
		error_ = error;
		ready_ = true;
		cond_.notify_all();
	}
};


template <class T>
class Future {	
	std::shared_ptr<FutureImpl<T>> impl_;
	
public:
	Future(): impl_(new FutureImpl<T>()) {}

	bool get(T *value) const {
		return impl_->get(value);
	}

	std::string error() const {
		return impl_->error();
	}

	void set(bool success, const T&& value,
			const std::string& error="") {
		impl_->set(success, value, error);
	}
};


class ThreadPool {

private: // Inner types
	
	class Task {
	public:
		// Execute the task
		virtual void run() = 0;

		// Cancel the task. Usually used to notify threads waiting
		// the execution result that the task has been canceled and
		// will never be executed.
		virtual void cancel() = 0;

		virtual ~Task() {}
	};

	template<class R>
	class TaskImpl : public Task {
		std::function<R()> fn_;
		Future<R> future_;
	public:
		explicit TaskImpl(std::function<R()>&& fn): fn_(fn) {}

		void run() {
			try {
				future_.set(true, fn_());
			} catch(std::exception& e) {
				future_.set(false, R(), e.what());
			} catch(...) {
				future_.set(false, R());
			}
		}

		void cancel() {
			future_.set(false, R(), 
				"task has been cancelled");
		}

		Future<R> get_future() {
			return future_;
		}
	};


	// TaskQueue: a blocking queue implementation w/o limit on its size.
	// 
	// 'add_task' enqueues tasks to the queue
	// 'next_task' dequeues tasks from the queue
	//
	// A thread trying to dequeue from an empty queue will be blocked 
	// until some other thread submits a task to the queue.
	//
	// Closing a TaskQueue will clear all tasks in the queue. If a queue is 
	// closed, 'add_task' does nothing and 'next_task' returns NULL.
	//
	class TaskQueue {
	private:
		std::queue<Task*> tasks_;  // the actural task queue
		std::mutex mutex_;   // lock protecting the data structure
		std::condition_variable cond_;  // for efficient waiting
		bool closed_;  // true if the task queue is closed
	
	public:
		TaskQueue(): closed_(false) {}
		
		// Find next task in the queue. It blocks until a task
		// is available. It returns NULL if the queue is closed.
		Task *next_task() {
			std::unique_lock<std::mutex> lock(mutex_);
			
			while (!closed_ && tasks_.empty())
				cond_.wait(lock);

			if (closed_) {
				return NULL;
			}

			Task* next = tasks_.front();
			tasks_.pop();
			return next;
		}
		
		// Returns true if the task is successfully added
		bool add_task(Task *task) {
			std::unique_lock<std::mutex> lock(mutex_);
			if (!closed_) {
				tasks_.push(task);
				cond_.notify_one();
			} else {
				task->cancel();
				delete task;
			}
			return !closed_;
		}

		// Close the task queue
		void close() {
			std::unique_lock<std::mutex> lock(mutex_);
			
			// clear tasks in the queue
			while (!tasks_.empty()) {
				Task *task = tasks_.front();
				tasks_.pop();
				task->cancel();
				delete task;
			}
			
			// set the queue closed and wake up all threads 
			// waiting on 'cond_'
			closed_ = true;
			cond_.notify_all();
		}

		// Returns true if the task queue has been closed
		bool closed() {
			std::unique_lock<std::mutex> lock(mutex_);
			return closed_;
		}
	
	};
	
	// Routine for worker threads to execute tasks.
	static void run_tasks(TaskQueue& task_queue) {
		while (!task_queue.closed()) { // test if the pool is closed
			// look for a task to execute
			Task *next = task_queue.next_task();
			if (next != NULL) { 
				next->run();
				delete next; 
			}
		}
	}

private:
	std::vector<std::thread> threads_; // worker threads
	int nthreads_;                     // # of worker threads
	TaskQueue task_queue_;             // the associated task queue

public:	
	explicit ThreadPool(int nthreads): nthreads_(nthreads) {
		for (int i = 0; i < nthreads; ++i) {
			threads_.emplace_back([this](){
					run_tasks(this->task_queue_);
				});
		}
	}

	// Sumbit a task to the thread pool. fn will be extected
	// asynchronously in one of the worker threads of the pool.
	template <class Fn, class... Args>
	auto async_apply(Fn&& fn, Args&&... args)
		-> Future<typename std::result_of<Fn(Args...)>::type> {
 		using R = typename std::result_of<Fn(Args...)>::type;
    		auto f = std::bind(std::forward<Fn>(fn), 
				std::forward<Args>(args)...);
		TaskImpl<R>* task = new TaskImpl<R>(f);
		task_queue_.add_task(task);
		return task->get_future();
	}

	// Close the thread pool. After the pool is closed, no tasks
	// would be executed except the running ones.
	void close() {
		task_queue_.close();
	}

	// Return true if the thread pool has already been closed.
	bool closed() {
		return task_queue_.closed();
	}
	
	// Wait all worker threads to terminate. 
	void join() {
		for (int i = 0; i < nthreads_; ++i)
			threads_[i].join();
	}

private:
	// Make ThreadPool non-copyable.
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
};

#endif  // THREAD_POOL_H_
