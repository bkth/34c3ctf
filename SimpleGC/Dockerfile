FROM ubuntu:17.10
RUN apt-get -y update
RUN apt-get -y install xinetd gdb

RUN groupadd -g 1000 sgc && useradd -g sgc -m -u 1000 sgc -s /bin/bash

RUN mkdir /chall

COPY xinetd.conf /etc/xinetd.d/chall
COPY sgc /chall/sgc
COPY flag /flag

#USER sgc
CMD xinetd -d -dontfork
#CMD /bin/bash
