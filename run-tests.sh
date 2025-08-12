#!/bin/bash

set -e -u

got_output_file=$(mktemp)

cleanup() {
    rm -f "$got_output_file"
}

trap cleanup EXIT

for sql in tests/*.sql; do
    sqlite3 :memory: ".read $sql" > "$got_output_file"
    diff --brief "$got_output_file" tests/$(basename "$sql" .sql).output
done
