####A. TAS Spinlock
	
	class TASSpinLock {
	public:
    	void Lock() {
        	while (locked_.exchange(true, std::memory_order_acq_rel)) {
            	std::this_thread::yield();
        	}
    	}

    	void Unlock() {
        	locked_.store(false, std::memory_order_release);
    	}

	private:
    	std::atomic<bool> locked_{false};
	};
1. В данном случае мы хотим упорядочить неатомарные чтения и записи в критические секции.
2. Упорядочивания **release/acquire** будет достаточно, т.к. синхронизация между потоками происходит на одном атомике `locked_`. Операция освобождения (`store`) в потоке, завершившим работу с критической секцией, синхронизируется-с операцией захвата (`exchange`) в потоке, получившем доступ к критической секции. Таким образом мы гарантируем возникновение необходимых стрелок **happens-before**, т.е. при следующем чтении из критической секции мы увидим записи в предыдущей. При этом у нас `Unlock()` первого потока синхронизуется-с `Lock()` второго, что гарантирует взаимное исключение. 
3. Режим упорядочивания **relaxed** не подходит, т.к. из того, что один поток записал в своем вызове `Lock()` в переменную `locked_` значение `true`, не будет следовать, что другие потоки прочитают именно последнее записанное значение -> несколько потоков смогут зайти в критическую секцию и взаимное исключение нарушается.