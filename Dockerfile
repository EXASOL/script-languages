FROM ubuntu:18.04

COPY ext/01_nodoc /etc/dpkg/dpkg.cfg.d/01_nodoc

RUN apt-get -y update && \
    apt-get -y install --no-install-recommends\
        locales \
        python \
        python3-pip \
        python3-wheel \
        python3-setuptools \
        git \
        bash \
        curl && \
    locale-gen en_US.UTF-8 && \
    update-locale LC_ALL=en_US.UTF-8 && \
    apt-get -y clean && \
    apt-get -y autoremove && \
    ldconfig

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
RUN pip3 install pipenv
RUN virtualenv --python=python3 /venv
#RUN echo "source /venv/bin/activate" >> ~/.bashrc
#RUN echo "source /venv/bin/activate" >> ~/.bash_profile
COPY Pipfile /root/Pipfile
RUN bash -c "source /venv/bin/activate && cd /root && pipenv --python python3 install"
