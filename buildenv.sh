#!/bin/bash
chmod +x judge_client
cp judge_client /usr/bin
useradd -uid 1535 judge
mkdir /home/judge/
mkdir /home/judge/etc
mkdir /home/judge/data
mkdir /home/judge/log
mkdir /home/judge/run

echo "LANG=C /usr/bin/socket_service" > /etc/init.d/oj_service
chmod -x /etc/init.d/judged
ls -s /etc/init.d/oj_service /etc/rc2.d/S93oj_service
ls -s /etc/init.d/oj_service /etc/rc3.d/S93judged

