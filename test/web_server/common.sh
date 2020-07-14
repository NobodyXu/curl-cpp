container_name=test-libcurl-cpp

has_command() {
    $1 -v >/dev/null 2>&1
    echo $?
}

if [ $(has_command docker) -ne 127 ]; then
    DOCKER=docker
elif [ $(has_command podman) -ne 127 ]; then
    DOCKER=podman
else
    echo Neither docker nor podman is installed on your computer!
    exit 1
fi

container_up() {
    podman ps --format "{{.Names}}" --no-trunc | grep -q $1
}
has_container() {
    podman ps -a --format "{{.Names}}" --no-trunc | grep -q $1
}
