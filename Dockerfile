FROM nvidia/cuda:11.7.0-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive  
ENV CUDA_ARCHS "Pascal"

RUN apt-get update && apt-get -y install \
    build-essential \
    git \
    cmake \
    libglm-dev \
    libflann-dev

# Build and install Theia
RUN apt-get -y install \
    libeigen3-dev \
    libcurl4-openssl-dev \
    libceres-dev \
    libgflags-dev \
    libopenimageio-dev \
    rapidjson-dev \
    librocksdb-dev \
    freeglut3-dev 

RUN git clone https://github.com/matevz-ap/TheiaSfM.git

RUN cd TheiaSfM && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 && \
    make install

RUN apt-get -y install \
    libboost-all-dev \
    libfreeimage-dev \
    libglew-dev \
    qtbase5-dev \
    libcgal-dev 

RUN git clone https://github.com/matevz-ap/colmap.git
RUN cd colmap && \
	mkdir build && \
	cd build && \
	cmake .. -DCUDA_ARCHS="${CUDA_ARCHS}" && \
	make -j4 && \
	make install

# Build OpenMVS
RUN apt-get -y install \
    libopencv-dev 

RUN git clone https://github.com/cdcseacave/VCG.git vcglib
RUN git clone https://github.com/cdcseacave/openMVS.git

RUN mkdir openMVS_build && \
    cd openMVS_build && \
    cmake . ../openMVS -DCMAKE_BUILD_TYPE=Release -DCMAKE_LIBRARY_PATH=/usr/local/cuda/lib64/stubs -DVCG_ROOT="../vcglib" && \
    make -j2 && \
    make install

RUN apt-get -y install \
    libcurl4-openssl-dev \
    libglm-dev \
    libflann-dev \
    libxinerama-dev \
    libxcursor-dev \
    redis-server \
    python3-pip

RUN export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda-11.2/compat

ADD ./sources/libigl /root/Sources/libigl

RUN apt-get -y install redis-server \ 
    python3-pip

COPY requirements.txt /tmp/requirements.txt
RUN pip install -r /tmp/requirements.txt

COPY . /app/

RUN cd app && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 

WORKDIR app

RUN apt-get -y install libatlas-base-dev

CMD ["python3","-u","server.py"]
