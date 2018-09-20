####B. Ring Buffer
	
	#pragma once

	#include <atomic>
	#include <vector>

	template <typename T>
	class SPSCRingBuffer {
	 public:
    	explicit SPSCRingBuffer(const size_t capacity)
        	: buffer_(capacity + 1) {
    	}

    	bool Publish(T element) {
        	const size_t curr_head = head_.load(std::memory_order_acquire);
        	const size_t curr_tail = tail_.load(std::memory_order_relaxed);

        	if (Full(curr_head, curr_tail)) {
            	return false;
        	}

        	buffer_[curr_tail] = element; 								//(1)
        	tail_.store(Next(curr_tail), std::memory_order_release);
        	return true;
    	}

    	bool Consume(T& element) {
        	const size_t curr_head = head_.load(std::memory_order_relaxed);
        	const size_t curr_tail = tail_.load(std::memory_order_acquire);

        	if (Empty(curr_head, curr_tail)) {
            	return false;
        	}

        	element = buffer_[curr_head];								//(2)
        	head_.store(Next(curr_head), std::memory_order_release);
        	return true;
    	}

	 private:
    	bool Full(const size_t head, const size_t tail) const {
    	    return Next(tail) == head;
    	}

    	bool Empty(const size_t head, const size_t tail) const {
        	return tail == head;
    	}

    	size_t Next(const size_t slot) const {
        	return (slot + 1) % buffer_.size();
    	}

	 private:
	    std::vector<T> buffer_;
    	std::atomic<size_t> tail_{0};
    	std::atomic<size_t> head_{0};
	};
1. Необходимо упорядочить операции (1) и (2) для корректной работы с очередью.
2.	* `tail_.store()` в `Publish()` синхронизируется-с `tail_.load()` в `Consume`,  возникает (1) *-happens-before->* (2), что позволяет потребителю извлекать добавленные элементы. Есть синхронизация между `head_.store()` и `head_.load()` между двумя вызовами `Consume()`, обеспечивающая прогресс при извлечении.
	* `head_.store()` в `Consume()` синхронизируется-с `head_.load()` в `Publish()`, позволяя добавлять новые элементы (очередь освобождается). Также есть синхронизация между `tail_.store()` и `tail_.load()` между двумя вызовами `Publish()`, обеспечивающая прогресс при добавлении.
	* Добавление/извлечение элемента происходит до изменения `tail_`/`head_` (это гарантирует **acquire/release** упорядочивание -> невозможны ситуации, когда мы обращаемся к еще не существующему элементу / перезаписываем еще не извлеченный.
3.	* Рассматриваем метод `Publish()`: у нас только один *Producer* и `tail_` изменяется только в методе `Publish()`, следовательно синхронизация при загрузке `tail_` будет происходить только в одном потоке и мы можем использовать семантику **relaxed**.
	* Аналогично для загрузки `head_` в методе `Consume()`.
	* **acquire/release** является достаточным, т.к. гарантирует возникновение всех необходимых стрелок, описанных в пункте 2.
	* Дальнейшее ослабление невозможно - нарушится порядок **happens-before**, который мы построили в первом пункте, т.к. мы будем работать с атомиками в разных потоках.