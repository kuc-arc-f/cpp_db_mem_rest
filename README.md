# db_mem1

 Version: 0.9.1

 date    : 2026/10/04
 
 update :

***

C++ DB Server Rest API , memory Database SQLite

* LLVM CLang
* cpp-httplib
* sqlite3 use

***
### related

https://github.com/yhirose/cpp-httplib

***
* LIB add
```
sudo apt update
sudo apt-get install libsqlite3-dev
sudo apt-get install nlohmann-json3-dev
sudo apt install libspdlog-dev libfmt-dev
```

***
* table add
```
sqlite3 ./data/backup.db < table.sql
```

***
* build
```
make all
```

* start , localhost:8888

```
./start.sh

```

***
* add
* action_name : action type
* table: tableName
* sql: sql text
```
curl -X POST http://localhost:8888/api/update \
  -H "Content-Type: application/json" \
  -d "{\"action_name\": \"update\", \"table\": \"temp\", \"sql\": \"INSERT INTO temp (title) VALUES ('title1');\"}"

```

* Select
* action_name : action type
* table: tableName
* sql: sql text
```
curl -X POST http://localhost:8888/api/select \
  -H "Content-Type: application/json" \
  -d '{"action_name": "select", "table": "temp", "sql": "SELECT * FROM temp;"}'
```

* vector delete
* action_name : action type
* table: tableName
* sql: sql text

```
curl -X POST http://localhost:8888/api/update \
  -H "Content-Type: application/json" \
  -d "{\"action_name\": \"update\", \"table\": \"temp\", \"sql\": \"DELETE FROM temp WHERE id=2;\"}"
```

***
### blog

https://zenn.dev/knaka0209/scraps/dc4dec763571f3

