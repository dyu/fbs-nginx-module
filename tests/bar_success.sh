#!/bin/sh

PORT=$1
[ -n "$PORT" ] || PORT=8082

curl -H "Content-Type: application/json" \
    -d '{"title":"hey","baz":{"kind":2},"foo":{"id":3,name:"foo"}}' \
    http://127.0.0.1:$PORT/bar
