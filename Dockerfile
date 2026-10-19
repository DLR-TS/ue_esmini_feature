FROM ghcr.io/epicgames/unreal-engine:dev-5.7.4

RUN sudo apt update\
&& sudo apt install wget cmake -y\
&& git clone https://github.com/esmini/esmini.git\
&& cd esmini\
&& git checkout v3.5.0

RUN cd esmini && mkdir build && cd build\
&& export TOOLCHAIN=/home/ue4/UnrealEngine/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v26_clang-20.1.8-rockylinux8/x86_64-unknown-linux-gnu\
&& export CC=$TOOLCHAIN/bin/clang\
&& export CXX=$TOOLCHAIN/bin/clang++\
&& export PATH=$TOOLCHAIN/bin:$PATH\
&& cmake .. \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_TESTING=OFF \
  -DUSE_OSG=OFF \
  -DUSE_IMPLOT=OFF \
  -DUSE_SUMO=OFF \
  -DUSE_OSI=OFF \
  -DCMAKE_C_COMPILER=$CC \
  -DCMAKE_CXX_COMPILER=$CXX \
  -DCMAKE_CXX_FLAGS="-O2 -stdlib=libc++"\
&& cmake --build . --target esminiLib_static -j$(nproc)

RUN cd /home/ue4/UnrealEngine/esmini\
&& { \
  printf 'CREATE build/EnvironmentSimulator/Libraries/esminiLib/libesminiLib_combined.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Libraries/esminiLib/libesminiLibs_static.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Modules/PlayerBase/libPlayerBase.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Modules/ScenarioEngine/libScenarioEngine.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Modules/CommonMini/libCommonMini.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Modules/Controllers/libControllers.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Modules/RoadManager/libRoadManager.a\n'; \
  printf 'ADDLIB build/EnvironmentSimulator/Modules/RoadManager/libpugixml_lib.a\n'; \
  printf 'ADDLIB build/externals/fmt/libfmt.a\n'; \
  printf 'SAVE\nEND\n'; \
} | ar -M\
&& ranlib build/EnvironmentSimulator/Libraries/esminiLib/libesminiLib_combined.a
