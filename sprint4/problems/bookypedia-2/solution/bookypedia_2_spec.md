# Bookypedia-2: Техническое задание и спецификация

Программа **Bookypedia** становится всё популярнее, и от пользователей поступили запросы на добавление новых возможностей:

- **Указывать у книг теги**, чтобы связать с ними дополнительную информацию, облегчающую поиск. Каждая книга может иметь произвольное количество тегов: строк длиной до 30 символов, несущих дополнительную информацию о книге.
- **Удалять книги**, которые были внесены по ошибке.
- **Удалять авторов**, если они также были внесены по ошибке.
- **Редактировать авторов и книги**.
- **Сделать интерфейс программы удобнее**.

---

## 1. База данных и общие требования

### Таблица `book_tags`
Для хранения тегов добавьте ещё одну таблицу `book_tags`, в которой разместите теги книг:
- `book_id` типа `uuid` — идентификатор книги, к которой относится тег.
- `tag` — строка длиной до 30 символов (`VARCHAR(30)`). Собственно, сам тег.

### Требования к реализации
- **Инициализация:** При старте программа должна проверить и создать необходимые для её работы таблицы, если они отсутствовали.
- **Расположение:** Своё решение разместите в каталоге `/sprint4/problems/bookypedia-2/solution`. В качестве основы используйте решение из предыдущей версии программы Bookypedia.
- **Транзакционность:** Каждая команда должна выполняться в рамках самостоятельной транзакции. Ошибки, возникшие во время выполнения команды, не должны приводить к изменениям.

---

## 2. Команды управления авторами

### Команда `DeleteAuthor`
Удаляет выбранного автора, все его книги и связанные с этими книгами теги. Книги других авторов и их теги не затрагиваются.

#### 1. Выбор из списка
```text
DeleteAuthor
1 Jack London
2 Joanne Rowling
Enter author # or empty line to cancel
1
ShowAuthors
1 Joanne Rowling
```

#### 2. Указание имени в аргументе команды
Можно указать имя удаляемого автора сразу в команде `DeleteAuthor`:
```text
ShowAuthors
1 Jack London
2 Joanne Rowling
DeleteAuthor Jack London
ShowAuthors
1 Joanne Rowling
```

#### 3. Обработка ошибок
Если выбранный автор уже был удалён в другом экземпляре приложения либо не существовал никогда, должно быть выведено сообщение `Failed to delete author`:
```text
ShowAuthors
1 Jack London
2 Joanne Rowling
DeleteAuthor Mr. Nobody
Failed to delete author
```

---

### Команда `EditAuthor`
Служит для редактирования выбранного автора. Можно указать имя автора в аргументе команды либо выбрать его из списка.

#### 1. Выбор из списка
```text
EditAuthor
Select author:
1 Jack London
2 Joanne Rowling
Enter author # or empty line to cancel
2
Enter new name:
J. K. Rowling
```

#### 2. Указание имени в аргументе команды
```text
EditAuthor Jack London
Enter new name:
John Griffith Chaney
ShowAuthors
1 J. K. Rowling
2 John Griffith Chaney
```

#### 3. Обработка ошибок
Если пользователь указал имя несуществующего автора либо в процессе ввода данных автор был удалён параллельно запущенным экземпляром программы, программа должна выдать сообщение:
```text
Failed to edit author
```

---

## 3. Команды управления книгами

### Обновление команды `AddBook`
В команду `AddBook` вносятся следующие изменения:
1. Добавлена возможность ввести имя автора напрямую либо выбрать автора из предложенного списка.
2. Если пользователь ввёл имя автора вручную, и среди авторов такого нет, программа должна предложить автоматически добавить автора:
   ```text
   AddBook 1906 White Fang
   Enter author name or empty line to select from list:
   Jack London
   No author found. Do you want to add Jack London (y/n)?
   y
   ```
   - Если пользователь ввёл ответ, отличный от `Y` или `y`, добавление книги отменяется и выводится сообщение: `Failed to add book`.
   - Если пользователь согласился добавить автора, в таблицу авторов добавляется новый автор и ввод данных о книге продолжается.
3. Программа запрашивает ввод тегов (разделяются запятыми):
   ```text
   Enter tags (comma separated):
   adventure, dog,   gold   rush  ,  dog,,dogs
   ```
   - Допускается нулевое количество тегов у книги.
4. **Нормализация тегов:** перед добавлением теги приводятся к нормализованному виду:
   - Пробелы в начале и в конце тега удаляются.
   - Лишние пробелы между словами тега схлопываются в один (например, `"gold   rush"` $	o$ `"gold rush"`).
   - Пустые теги и дубликаты существующих тегов удаляются (в примере выше остаётся один `"dog"`, а пустой тег между `dog` и `dogs` удаляется).
   - Результат для примера выше: `adventure`, `gold rush`, `dog`, `dogs`.
5. Как и в предыдущей версии, пользователь может оставить строку автора пустой и выбрать автора книги из списка.

---

### Обновление команды `ShowBooks`
Команда `ShowBooks` кроме названия книги и года публикации должна выводить её автора.

**Формат вывода:**
```text
ПорядковыйНомер НазваниеКниги by ИмяАвтора, ГодПубликации
```

**Правила сортировки:**
1. По названию книги (алфавитный порядок);
2. Книги с одинаковым названием — по имени автора;
3. Книги с одинаковым названием и автором — по возрастанию года публикации.

**Пример:**
```text
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
```

---

### Команда `ShowBook`
Выводит подробную информацию о книге: автор, название, год публикации, теги (если есть). Можно указать название книги в самой команде либо выбрать из списка.

- Если книг с введённым названием несколько, программа предлагает выбрать нужную из списка.
- Если у книги есть теги, выводится строка `Tags: `, за которой следуют теги в **алфавитном порядке**, разделённые запятой с пробелом.
- Если тегов нет, строка `Tags:` не выводится вовсе.
- Если книги с введённым названием нет, ничего не выводится.

#### Примеры использования:

```text
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906

ShowBook White Fang
Title: White Fang
Author: Jack London
Publication year: 1906
Tags: adventure, dog, gold rush

ShowBook The Cloud Atlas
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
Enter the book # or empty line to cancel:
2
Title: The Cloud Atlas
Author: Liam Callanan
Publication year: 2004

ShowBook
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
Enter the book # or empty line to cancel:
3
Title: White Fang
Author: Jack London
Publication year: 1906
Tags: adventure, dog, gold rush

ShowBook Oliver Twist
```

---

### Команда `DeleteBook`
Позволяет удалить книгу, указав её название напрямую или выбрав из списка. 

- Если книг с введённым названием несколько либо название не было указано в аргументе команды, программа выводит список подходящих книг и предлагает выбрать номер.
- В случае ошибки удаления книги программа должна вывести сообщение: `Failed to delete book`.

**Пример:**
```text
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906

DeleteBook The Cloud Atlas
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
Enter the book # or empty line to cancel:
2

ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 White Fang by Jack London, 1906

DeleteBook
1 The Cloud Atlas by David Mitchell, 2004
2 White Fang by Jack London, 1906
Enter the book # or empty line to cancel:
1
```

---

### Команда `EditBook`
Позволяет отредактировать книгу. Название можно передать в аргументе либо выбрать книгу из списка. Если найдено несколько книг с одинаковым названием, программа предлагает выбрать нужную из списка.

- Пользователь может по очереди обновить:
  - Название книги (пустая строка — оставить текущее);
  - Год публикации (пустая строка — оставить текущий);
  - Теги книги (вводятся через запятую; пустая строка или новые теги заменяют старые согласно правилам нормализации).
- Если введённая книга отсутствует либо была удалена из другого экземпляра программы, выводится: `Book not found`.

**Пример:**
```text
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fan by Jack London, 1906

EditBook White Fan
Enter new title or empty line to use the current one (White Fan):
White Fang
Enter publication year or empty line to use the current one (1906):

Enter tags (current tags: adventure, cat, gold rush):
adventure, gold rush, dog

ShowBook White Fang
Title: White Fang
Author: Jack London
Publication year: 1906
Tags: adventure, dog, gold rush

EditBook
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
Enter the book # or empty line to cancel:
3
Enter new title or empty line to use the current one (White Fang):

Enter publication year or empty line to use the current one (1906):

Enter tags (current tags: adventure, dog, gold rush):
adventure, dog, gold rush, wolf
```

---

## 4. Тестирование

Автоматические тесты проверят работу вашей программы:
1. Настроят структуру базы данных и переменные окружения перед запуском.
2. Запустят программу несколько раз, последовательно подавая на вход описанные в задании команды.
3. Проверят строгое соответствие выводимых данных ожидаемым результатам, форматам и сообщениям об ошибках.
