#!/bin/bash -ex

source $(dirname $0)/stop_nginx.sh

if has_container $container_name; then
    $DOCKER rm $container_name
else
    echo No $container_name exists
fi

echo
