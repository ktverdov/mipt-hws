####C. Double Checked Locking
	#pragma once

	#include <atomic>
	#include <mutex>
	#include <functional>

	template <typename T>
	class LazyValue {
     using Factory = std::function<T*()>;
	 public:
    	explicit LazyValue(Factory create)
    		: create_(create) {
    	}

    	T& Get() {
        	// double checked locking pattern
        	T* curr_ptr = ptr_to_value_.load(std::memory_order_acquire);		//(1)
        	if (curr_ptr == nullptr) {
        	    std::lock_guard<std::mutex> guard(mutex_);
        	    curr_ptr = ptr_to_value_.load(std::memory_order_relaxed);		//(2)
            	if (curr_ptr == nullptr) {
                	curr_ptr = create_();
                	ptr_to_value_.store(curr_ptr, std::memory_order_release);
            	}
        	}
       		return *curr_ptr;
    	}

    	~LazyValue() {
        	if (ptr_to_value_.load() != nullptr) {
        	    delete ptr_to_value_;
        	}
    	}

	 private:
    	Factory create_;
    	std::mutex mutex_;
    	std::atomic<T*> ptr_to_value_{nullptr};
	};
1. Должны быть упорядочены между различными потоками `std::lock_guard<std::mutex> guard(mutex_)` и `curr_ptr = create_()`.
2.	> * `ptr_to_value_.store()` в победившем потоке синхронизуется-с (1) и (2) в других потоках. Здесь достаточно использовать **acquire/release** упорядочивание, т.к. потоки работают с одним атомиком. 
	> * Но недостаточно **relaxed** семантики, т.к. тогда нам `load()` может вернуть неверное значение - `nullptr`, хотя другой поток уже создал объект и обновил `ptr_to_value_`.
		* если мы сделаем в (1) **relaxed**, то будем бесполезно захватывать мьютекс.
		* если изменим в (2), то будем создавать объект несколько раз.

Приведенные выше рассуждения верны для (1), но не верны для (2) загрузки. Там мы можем использовать **relaxed** упорядочивание, т.к. (2) `load()` и `store()` находятся под мьютексом, который будет гарантировать видимость изменений, сделанных `store()`, для других потоков, попавших в критическую секцию.