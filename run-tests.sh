#!/bin/bash

set -e -u

failure_count=0

fail() {
    echo "FAIL: $1 $2" >&2
    (( ++failure_count ))
}

got_output_file=$(mktemp)
got_error_file=$(mktemp)

cleanup() {
    rm -f "$got_output_file" "$got_error_file"
}

trap cleanup EXIT

for sql in tests/*.sql; do
    test_name=$(basename "$sql" .sql)
    expected_output_file="tests/${test_name}.output"
    expected_error_file="tests/${test_name}.error"

    expected_exit_status=0
    if [[ -f "$expected_error_file" ]] ; then
        expected_exit_status=1
    else
        expected_error_file=/dev/null
    fi

    sqlite3 :memory: ".read $sql" > "$got_output_file" 2> "$got_error_file" && got_exit_status=0 || got_exit_status=1

    if [[ $got_exit_status -ne $expected_exit_status ]] ; then
        fail "$test_name" 'exit status mismatch'
    fi

    if ! diff --brief "$got_output_file" "$expected_output_file" ; then
        fail "$test_name" 'standard output mismatch'
    fi

    if ! diff --brief "$got_error_file" "$expected_error_file" ; then
        fail "$test_name" 'standard error mismatch'
    fi
done

if [[ $failure_count -gt 0 ]] ; then
    exit 1
fi
