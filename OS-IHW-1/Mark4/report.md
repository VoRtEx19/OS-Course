# Отчёт по программе на 4 балла

Автор: Лев Алексеевич Светличный, БПИ 225

Вариант 36

Условие задачи:
Разработать программу, которая на основе анализа двух ASCII–строк формирует на выходе строку, 
содержащую символы, присутствующие в одной или другой (объединение символов). Каждый символ 
в соответствующей выходной строке должен встречаться только один раз. Входными и выходными 
параметрами являются имена трех файлов, задающих входные и выходную строки.

Схема решения задачи:

- Программа на вход принимает три пути к файлам
- Создается три pipe - неименованных канала, два для работы со считыванием из файлов, один для записи
- Создаётся 4! дочерних процесса (два для считывания каждого файла, 
один для обработки данных из считанных файлов, один для записи в выходной файл)

NB. использую четыре процесса, т.к. если для обоих входных файлов использовать один процесс, 
то смысл наличия двух файлов теряется.

Результаты работы программы - см. приведенные файлы
