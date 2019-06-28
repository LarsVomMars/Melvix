#!/usr/bin/env bash

export source=$(pwd)

sudo useradd -m melvix
sudo mkdir -p /home/melvix/os
sudo cp -r boot etc kernel.conf /home/melvix/os
sudo chown -R melvix /home/melvix
sudo cp bootstrap.sh /home/melvix

tput civis
while :;do for s in / - \\ \|; do printf "\r$s";sleep .1;done;done &
trap "kill $!" EXIT
sudo -i -u melvix bash bootstrap.sh ${source}
kill $! && trap " " EXIT
tput cnorm

export MELVIX=/home/melvix/os
sudo find ${MELVIX}-copy/{,usr/}{bin,lib,sbin} -type f -exec sudo strip --strip-debug '{}' ';'
sudo find ${MELVIX}-copy/{,usr/}lib64 -type f -exec sudo strip --strip-debug '{}' ';'
sudo chown -R root:root ${MELVIX}-copy
sudo chgrp 13 ${MELVIX}-copy/var/run/utmp ${MELVIX}-copy/var/log/lastlog
sudo mknod -m 0666 ${MELVIX}-copy/dev/null c 1 3
sudo mknod -m 0600 ${MELVIX}-copy/dev/console c 5 1
sudo chmod 4755 ${MELVIX}-copy/bin/busybox

sudo tar cfJ ${source}/melvix-build.tar.xz ${MELVIX}-copy/*