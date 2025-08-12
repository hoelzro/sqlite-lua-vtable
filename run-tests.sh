#!/bin/bash

set -e

for sql in tests/*.sql; do
    sqlite3 :memory: ".read $sql" > testcase-out.txt
    diff --brief testcase-out.txt tests/$(basename "$sql" .sql).output
done
