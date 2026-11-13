# lab2_concurrent_container

Лабораторная работа №2: конкурентный контейнер.

Каждый контейнер в цикле создаёт в общем томе файл с первым свободным именем
(`001`, `002`, ...), пишет в него свой случайный идентификатор и номер файла,
через секунду удаляет его и ещё через секунду повторяет. Поиск имени и создание
файла выполняются под `flock` на файле `/data/.lock`.

## Запуск

```sh
docker volume create shared
docker build -t concurrent .

# один экземпляр
docker run -d -v shared:/data concurrent

# 10 (или 50) экземпляров
for i in $(seq 10); do docker run -d -v shared:/data concurrent; done
```

Посмотреть файлы в томе:

```sh
docker run --rm -v shared:/data alpine:3.20 sh -c 'cd /data && grep . *'
```

Остановить все экземпляры:

```sh
docker rm -f $(docker ps -aq --filter ancestor=concurrent)
```
