#####TAS spinlock:

	bool try_lock()
	{
		return !locked.exchange(true);
	}
	
#####Ticket Spinlock:
	
Воспользуемся атомарной функцией
	
	bool compare_and_swap(*T obj, T expected, T desired);
которая сравнивает expected со значением в obj, и если они одинаковые, то заменяет значение в obj на desired.

Реализация:

	
	bool try_lock()
 	{
		return cas(&next_ticket, owner_ticket, next_ticket + 1);
	}

правильно работать не будет.

				T0								T1
			try_lock():  
		next_ticket + 1 = 1
											  lock():
								 this_ticket = 0, next_ticket = 1;
											   CS1
											unlock():
										 owner_ticket = 1;
	  	   cas(1, 1, 1):
		 next_ticket = 1
	поток входит в кр. секцию
											lock():
								 this_ticket = 1, next_ticket = 2
								 	поток входит в кр. секцию

Данная реализация try\_lock() не гарантирует взаимного исключения.

Исправленная реализация метода:

	bool try_lock()
	{
		int owner_ticket_copy = owner_ticket.load();
		return cas(&next_ticket, owner_ticket_copy, owner_ticket_copy + 1);
	}									
							