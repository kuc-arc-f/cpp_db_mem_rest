#!/bin/bash

sqlite3 ./data/backup.db .dump > ./data/backup.sql

./db_mem_rest_server
