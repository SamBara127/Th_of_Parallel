import queue

# Создание очереди
q = queue.Queue()

# Добавление элементов в очередь
q.put(1)
q.put(2)
q.put(3)

# Извлечение элементов из очереди
while not q.empty():
    print(q.get())