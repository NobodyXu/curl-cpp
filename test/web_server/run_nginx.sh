#!/bin/bash -ex

source $(dirname $0)/common.sh

if ! container_up $container_name; then
    if has_container $container_name; then
        echo $container_name already exists, now try to start it
        $DOCKER start $container_name
    else
        echo Creating $container_name
        $DOCKER run --name $container_name -p 8787:80 -d --mount=type=bind,src=$(dirname $0),dst=/usr/share/nginx/html,ro=true nginx:stable
    fi
else
    echo $container_name is already running!
fi

echo
