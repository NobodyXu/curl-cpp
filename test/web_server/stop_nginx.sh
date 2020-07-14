#!/bin/bash

source $(dirname $0)/common.sh

if container_up $container_name; then
    $DOCKER stop $container_name
else
    echo $container_name is already stopped!
fi
