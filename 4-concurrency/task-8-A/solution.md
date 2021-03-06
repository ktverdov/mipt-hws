####Задача A. Распределённый счётчик

#####1
 
* Докажем нелинеаризуемость счетчика, построив конкурентную историю, которую невозможно описать последовательной историей, соблюдающей частичный порядок <<sub>H .

"|" обозначают конец и начала метода.

 `T1: .(1)..| Add(2) |.(2)...............(3)......`

 `T2: .....................| Add(3) |.............`

 `T3: |.............Get()::result(3).............|`

`Add(2)` < `Add(3)`, значит мы можем поместить операцию `Get()` только в (1) -> `Get()::result(0)`, (2) -> `Get()::result(2)`, (3) -> `Get()::result(5)`. А наша программа вернула `3`, например, прочитав первую ячейку - `0` до первого `Add()`, заснув, и продолжив читать после завершения второго `Add()`.

Мы привели нелинеаризуемую историю вызовов -> объект не является линеаризуемым.

* Программа сможет такой счетчик отличить от атомарного - см. ситуацию выше. Возвращенное `Get()` значение не совпадает ни с одним возможным состоянием счетчика (ни с одной суммой).

#####2

Да, счетчик с `Insert()` будет линеаризуемым.

Возьмем в качестве точек линеаризации:

`Insert()` - инкремент, `Get()` - момент возвращения значения. 

Таким образом все возможные исполнения можно будет представить так (* - момент инкремента, | - результат `Get()`, возвращающий количество точек слева от него):

`....*.|..........|...........*.....`

`....*.|........*.|.................`

`.....*|....*.....|........*.....*..`

`......|..........|......*..........`

`.*..*.|..........|.....*.....*.....`

и всегда можно разместить `Get()` так, чтобы возвращенный им результат совпал с количеством завершенных `Increment()`. 