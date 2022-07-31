FROM nvidia/cuda:11.2.0-devel-ubuntu20.04

# Prevent stop building ubuntu at time zone selection.  
ENV DEBIAN_FRONTEND=noninteractive  

RUN apt-get update && apt-get install -y \
    git \
    cmake \
    build-essential \
    libboost-program-options-dev \
    libboost-filesystem-dev \
    libboost-graph-dev \
    libboost-system-dev \
    libboost-test-dev \
    # libeigen3-dev \
    libsuitesparse-dev \
    libfreeimage-dev \
    libmetis-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libglew-dev \
    qtbase5-dev \
    libqt5opengl5-dev \
    libcgal-dev 

# Eigen 3.4
RUN git clone https://gitlab.com/libeigen/eigen.git --branch 3.4 && \
    mkdir eigen_build && cd eigen_build && \
    cmake . ../eigen && \
    make && make install && \
    cd .. 

# ceres solver
RUN apt-get -y install \
    libatlas-base-dev \
    libsuitesparse-dev

RUN git clone https://github.com/ceres-solver/ceres-solver.git --branch 2.1.0
RUN cd ceres-solver && \
	mkdir build && \
	cd build && \
	cmake .. -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF && \
	make -j3 && \
	make install

ENV CUDA_ARCHS "Pascal"

# Build and install COLMAP
# RUN git clone https://github.com/colmap/colmap.git

# Build and install Theia
RUN apt-get -y install \
    librocksdb-dev \
    rapidjson-dev \
    freeglut3-dev \
    libjpeg-dev \
    libopenimageio-dev

RUN git clone https://github.com/matevz-ap/TheiaSfM.git

RUN cd TheiaSfM && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 && \
    make install

# Build OpenMVS
RUN apt-get -y install \
    libopencv-dev \
    libboost-iostreams-dev \
    libboost-program-options-dev \
    libboost-system-dev \
    libboost-serialization-dev \
    libcgal-qt5-dev \
    libtiff-dev \
    libglu1-mesa-dev \
    # optional
    freeglut3-dev \
    libglew-dev \
    libglfw3-dev

RUN git clone https://github.com/cdcseacave/VCG.git vcglib
RUN git clone https://github.com/cdcseacave/openMVS.git

RUN mkdir openMVS_build && \
    cd openMVS_build && \
    cmake . ../openMVS -DCMAKE_BUILD_TYPE=Release -DCMAKE_LIBRARY_PATH=/usr/local/cuda/lib64/stubs -DVCG_ROOT="../vcglib" && \
    make -j2 && \
    make install

RUN git clone https://github.com/matevz-ap/colmap.git
RUN cd colmap && \
	mkdir build && \
	cd build && \
	cmake .. -DCUDA_ARCHS="${CUDA_ARCHS}" && \
	make -j4 && \
	make install

RUN apt-get -y install \
    libcurl4-openssl-dev \
    libglm-dev \
    libflann-dev \
    libxinerama-dev \
    libxcursor-dev

ADD ./sources/libigl /root/Sources/libigl
RUN git clone https://github.com/matevz-ap/PTAM-SP-reconstruction.git

# RUN git clone https://github.com/Dav1dde/glad.git && \
#     cd glad && \
#     cmake ./ && \
#     make && \
#     cp -a inshortuuidclude /usr/local/

RUN cd PTAM-SP-reconstruction && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j2 

RUN apt-get -y install redis-server \ 
    python3-pip
RUN pip install -r requirements.txt

