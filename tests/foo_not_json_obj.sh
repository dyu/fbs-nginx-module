#!/bin/sh

PORT=$1
[ -n "$PORT" ] || PORT=8082

curl -H "Content-Type: application/json" \
    -d '[{"id":1,"name":"xyz","description":"xyz"}]' \
    http://127.0.0.1:$PORT/foo
