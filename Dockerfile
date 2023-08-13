FROM nvidia/cuda:11.7.1-devel-ubuntu22.04
ENV DEBIAN_FRONTEND=noninteractive  

COPY packages.txt /tmp/packages.txt
RUN apt-get update
RUN xargs -a /tmp/packages.txt apt-get -y install

RUN git clone https://github.com/matevz-ap/TheiaSfM.git
RUN cd TheiaSfM && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 && \
    make install

ADD ./patches/colmap.patch /tmp/colmap.patch
RUN git clone https://github.com/colmap/colmap
RUN cd colmap && \
    git checkout 3.7 && \
    git apply /tmp/colmap.patch && \
    mkdir build && \
	cd build && \
	cmake .. && \
	make -j3 && \
    make install

RUN git clone https://github.com/cdcseacave/VCG.git vcglib
RUN git clone https://github.com/cdcseacave/openMVS.git && cd openMVS && git checkout 3b2bb84 

RUN mkdir openMVS_build && \
    cd openMVS_build && \
    cmake . ../openMVS -DCMAKE_BUILD_TYPE=Release -DVCG_ROOT="../vcglib" -DCMAKE_LIBRARY_PATH=/usr/local/cuda/lib64/stubs && \
    make -j2 && \
    make install

ADD ./sources/libigl /root/Sources/libigl

COPY requirements.txt /tmp/requirements.txt
RUN pip install -r /tmp/requirements.txt

RUN export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda-11.7/compat

COPY . /app/

RUN cd app && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 

WORKDIR app

# CMD ["python3","-u","server.py"]