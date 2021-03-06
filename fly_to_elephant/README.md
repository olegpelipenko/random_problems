## Задание

### Необходимо реализовать алгоритм решения задачи «Как из мухи сделать слона».

Реализацию необходимо предоставить в виде исходного кода консольного приложения.
Реализация приложения должна проходить в соответствии c технологией разработки TDD.
Стиль написания кода должен соответствовать Camel notation, код логик и тестов должен быть качественно закомментирован. Любые оптимизации решения считаются преимуществом. В пояснительной записке к решению необходимо указать, какому шаблону проектирования оно соответствует. Реализацию (исходный код, Unit-тесты и пояснительную записку) выложить на GitHub и прислать ссылку.
Приложение необходимо реализовать на языке C++ (должно собираться компислятором GCC 4.9 или clang 3.5).

### Постановка задачи
Дано исходное и конечное слово равной длины. Длина исходных слов не ограничена.
Необходимо составить цепочку слов от исходного слова до конечного. Каждое следующее слово в цепочке может отличаться от предыдущего только одной буквой. Исходное, конечное и все промежуточные слова должны состоять из одинакового количества букв.
Все используемые слова обязательно должны содержаться в заранее определенном словаре.
На вход в программу подается:

1. Путь к текстовому файлу, в котором указано начальное и конечное слово. На первой строке указано начальное слово, на второй строке конечное


2. Путь к файлу, который содержит словарь. Слова в словаре указаны по одному на каждой строке. В словаре слова могут быть разной длины.
На выходе программа должна вывести в консоль путь от исходного слова к конечному, по одному слову на одной строке.

Пример

Начальное слово: КОТ

Конечное слово: ТОН

Словарь:

КОТ

ТОН

НОТА

КОТЫ

РОТ

РОТА

ТОТ

Решение
КОТ

ТОТ

ТОН


## Решение
Общее описание алгоритма:

* Читаем файл построчно: одна строка = одно слово, читаем только слова указанной длинны (длины исходного и результирующего слова одинаковы).
	
* Подготавливаем слова: каждое слово приводим к нижнему регистру.
	
* Строим направленный граф: перебираем все слова и сравниваем со всеми остальными, каждое слово отличное от другого на одну букву добавляем как грань.
	
* Поиск переходов: проходим по графу от начального слова до конечного всеми возможными вариантами, выводим самый короткий путь.

* Выводим итоговый путь в stdout.
	

### Компиляция: 
```
cmake CMakeLists.txt
make
```

Для компиляции нужен gtest.

### Пример использования:
```
./fly_to_elephant data/input.txt data/google-10000-english.txt
mail
mall
ball
bill
biol
bios
bias
bras
gras
grab
```

### Пояснения:
* Мне кажется, что я недостаточно закомментировал код, но я не понял в каком стиле это нужно было сделать и что конкретно дописать к функциям. Когда я начинал писать текст комментария я ловил себя на мысли, что просто повторяю её название другими словами.
* Никакого особого паттерна в этой реализации использовано не было. Была выделена сущность граф и несколько вспомогательных функций. Код алгоритма Дейкстры намеренно вынесен в отдельную функцию, чтобы в графе было только то, что имеет отношение именно к нему. В графе связи представлены индексами массива слов, дабы хранить вершины в одном месте и не задваивать информацию. Возможно можно было сделать некую сущность Solver, в которую собрать вспомогательные функции и сказать, что решения могут быть разные и каждое определяется в своей реализации, но мне это показалось излишним, потому что сейчас только одна реализация.
* Тестировал максимально на мегабайтных словарях. Если словари будут какими то особо большими, то решение хранить в памяти может не подойти и нужно будет работать с файлом по частям.
