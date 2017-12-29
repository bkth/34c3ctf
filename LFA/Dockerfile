FROM ubuntu:17.10
RUN apt-get -y update
RUN apt-get -y install xinetd git gcc vim autoconf make bison ruby python cgroup-tools

RUN groupadd -g 1000 lfa && useradd -g lfa -m -u 1000 lfa -s /bin/bash

RUN mkdir /chall
RUN mkdir /build
COPY sandbox_patch /build/sandbox_patch
# compile ruby
WORKDIR /build
RUN git clone https://github.com/ruby/ruby.git
WORKDIR /build/ruby
RUN git checkout a5ec07c73fb667378ed617da6031381ee2d832b0 
RUN git apply ../sandbox_patch
RUN autoconf
RUN ./configure
RUN make install
WORKDIR /build
COPY flag /flag
COPY server.py /chall/server.py
COPY start_server.sh /chall/start_server.sh
COPY LFA.so /usr/local/lib/ruby/site_ruby/2.4.0/x86_64-linux/LFA.so
RUN chmod +x /chall/server.py
RUN chmod +x /chall/start_server.sh
COPY xinetd.conf /etc/xinetd.d/chall
COPY starter.sh /chall/starter.sh
COPY pow.py /chall/pow.py

CMD /chall/starter.sh LFA 

