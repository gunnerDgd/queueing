#pragma once
#include <mutex>

#include <memory>
#include <atomic>

namespace queueing {
	template <std::size_t QueueSize, typename QueueType, typename QueueAllocator = std::allocator<QueueType >>
	class circular_queue
	{
	public:
		typedef QueueType  value_type;
		typedef QueueType* pointer   ;
		typedef QueueType& reference ;
		class   chain;
	
	public:	
		class reader;
		class reader_lock;
		
		class writer;
		class writer_lock;

	private:
		chain*		__M_queue_chain;

		reader      __M_queue_reader;
		reader_lock __M_queue_rlock ;

		writer		__M_queue_writer;
		writer_lock __M_queue_wlock ;
	};

	template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
	class circular_queue<QueueSize, QueueType, QueueAllocator>::chain
	{
		typedef circular_queue<QueueSize, QueueType, QueueAllocator>::chain		 chain_type;
		typedef circular_queue<QueueSize, QueueType, QueueAllocator>::chain*	 chain_pointer;
		typedef circular_queue<QueueSize, QueueType, QueueAllocator>::value_type value_type;
	public:
		chain(chain_pointer prev, chain_pointer next, value_type* ptr) : __M_chain_prev (prev), 
																		 __M_chain_next (next), 
																		 __M_chain_value(ptr) {  }

	public:
		value_type* operator->() { return  __M_chain_value; }
		value_type* operator* () { return *__M_chain_value; }

	public:
		chain_pointer next()     { return __M_chain_next  ; }
		chain_pointer prev()     { return __M_chain_prev  ; }
		
	private:
		value_type*   __M_chain_value;
		chain_pointer __M_chain_prev, __M_chain_next;
	};

	template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
	class circular_queue<QueueSize, QueueType, QueueAllocator>::reader
	{
		friend class circular_queue<QueueSize, QueueType, QueueAllocator>;
		friend class circular_queue<QueueSize, QueueType, QueueAllocator>::writer;
		
		typedef      circular_queue<QueueSize, QueueType, QueueAllocator>		 queue_type;
		typedef      circular_queue<QueueSize, QueueType, QueueAllocator>::chain chain_type;

		reader();
	public:
		static reader& from_queue(queue_type& q) { return q.__M_queue_reader; }
	public:
		queue_type::reference operator* ();
		reader&				  operator++();
		
		bool				  operator==(queue_type::reader&);
		bool				  operator==(queue_type::writer&);
		
		bool				  operator!=(queue_type::reader&);
		bool				  operator!=(queue_type::writer&);

	private:
		chain_type			   * __M_reader_chain;
		queue_type::reader_lock& __M_reader_lock ;
	};

	template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
	class circular_queue<QueueSize, QueueType, QueueAllocator>::writer
	{
		friend class circular_queue<QueueSize, QueueType, QueueAllocator>;
		friend class circular_queue<QueueSize, QueueType, QueueAllocator>::reader;
		
		typedef      circular_queue<QueueSize, QueueType, QueueAllocator>		 queue_type;
		typedef      circular_queue<QueueSize, QueueType, QueueAllocator>::chain chain_type;

		writer();
	public:
		static writer& from_queue(queue_type& q) { return q.__M_queue_writer; }
	public:
		writer&	    operator= (queue_type::reference);

		bool		operator==(queue_type::reader&);
		bool		operator==(queue_type::writer&);

		bool		operator!=(queue_type::reader&);
		bool		operator!=(queue_type::writer&);
	
	private:
		chain_type			   * __M_writer_chain;
		queue_type::writer_lock& __M_writer_lock ;
	};

	template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
	class circular_queue<QueueSize, QueueType, QueueAllocator>::reader_lock
	{
	public:
		reader_lock () : __M_rlock_entity(ATOMIC_FLAG_INIT) {  }
		~reader_lock()										{  }

	public:
		void lock  ();
		void unlock();

	private:
		std::atomic_flag __M_rlock_entity;
	};

	template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
	class circular_queue<QueueSize, QueueType, QueueAllocator>::writer_lock
	{
	public:
		writer_lock () : __M_wlock_entity(ATOMIC_FLAG_INIT) {  }
		~writer_lock()										{  }

	public:
		void lock  ();
		void unlock();

	private:
		std::atomic_flag __M_wlock_entity;
	};
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
void queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader_lock::lock()
{
	while (__M_rlock_entity.test_and_set(std::memory_order_acquire))
		while (__M_rlock_entity.test(std::memory_order_relaxed));
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
void queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader_lock::unlock()
{
	__M_rlock_entity.clear(std::memory_order_release);
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
void queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer_lock::lock()
{
	while (__M_wlock_entity.test_and_set(std::memory_order_acquire))
		while (__M_wlock_entity.test(std::memory_order_relaxed));
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
void queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer_lock::unlock()
{
	__M_wlock_entity.clear(std::memory_order_release);
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reference
	queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader::operator* () { return  **__M_reader_chain; }

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader&
	queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader::operator++()
{
	std::lock_guard<queue_type::reader_lock> rlock(__M_reader_lock);
												   __M_reader_chain = __M_reader_chain->next();
	return *this;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader::operator==(queue_type::reader& cmp)
{
	std::lock_guard<queue_type::reader_lock> rlock(__M_reader_lock);
	return				   cmp.__M_reader_chain == __M_reader_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader::operator==(queue_type::writer& cmp)
{
	std::lock_guard<queue_type::reader_lock> rlock    (__M_reader_lock);
	std::lock_guard<queue_type::reader_lock> wlock(cmp.__M_writer_lock);

	return				   cmp.__M_writer_chain == __M_reader_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader::operator!=(queue_type::reader& cmp)
{
	std::lock_guard<queue_type::reader_lock> rlock(__M_reader_lock);
	return				   cmp.__M_reader_chain != __M_reader_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::reader::operator!=(queue_type::writer& cmp)
{
	std::lock_guard<queue_type::reader_lock> rlock    (__M_reader_lock);
	std::lock_guard<queue_type::reader_lock> wlock(cmp.__M_writer_lock);

	return cmp.__M_writer_chain != __M_reader_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer&
	queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer::operator= (queue_type::reference r) { **__M_queue_chain = r; return *this; }

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer::operator==(queue_type::reader& cmp)
{
	return cmp.__M_reader_chain == __M_writer_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer::operator==(queue_type::writer& cmp)
{
	return cmp.__M_writer_chain == __M_writer_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer::operator!=(queue_type::reader& cmp)
{
	return cmp.__M_reader_chain != __M_writer_chain;
}

template <std::size_t QueueSize, typename QueueType, typename QueueAllocator>
bool queueing::circular_queue<QueueSize, QueueType, QueueAllocator>::writer::operator!=(queue_type::writer& cmp)
{
	return cmp.__M_writer_chain != __M_writer_chain;
}