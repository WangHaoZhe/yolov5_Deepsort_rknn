## 对原项目的改动

原项目地址: https://github.com/Zhou-sx/yolov5_Deepsort_rknn/

主要改动为:
1. 推理模型更改为官方的yolov5s-640-640.rknn
2. 修正源代码的resize部分的问题
3. post_process改为int8

TODO:
1. 接入摄像头视频流
2. 配置为RGA3

## 平台

开发板: Orangepi 5 Max 16G
使用的系统镜像: Ubuntu Orangepi5max_1.0.0_ubuntu_focal_desktop_xfce_linux5.10.160.img

## 环境配置

0. 安装系统

对照OrangePi_5_Max_RK3588_用户手册_v1.3中第2章的内容烧录tf卡并启动系统

1. 安装cmake

```shell
sudo apt-get install cmake
```

2. 安装x264

```shell
git clone git://git.videolan.org/x264.git
cd x264
./configure --prefix=/usr/local/x264 --disable-opencl --enable-static --enable-shared
sudo make -j8
sudo make install
```

验证:
```shell
cd /usr/lib/x264/lib
file libx264.so.164
```
可以看到输出

添加到环境变量:
将下面一行添加到/etc/profile末尾
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/x264/lib/pkgconfig
保存后刷新
```shell
source /etc/profile
```

3. 安装libdrm

```shell
wget https://dri.freedesktop.org/libdrm/libdrm-2.4.89.tar.bz2
tar -jxvf libdrm-2.4.89.tar.bz2
cd libdrm-2.4.89
./configure --prefix=/usr/local/libdrm --host=aarch64-linux-gnu
make -j8
sudo make install
```

4. 安装ffmpeg

```shell
wget https://ffmpeg.org/releases/ffmpeg-4.1.3.tar.bz2
tar -xjf ffmpeg-4.1.3.tar.bz2
cd ffmpeg-4.1.3
./configure --enable-shared --enable-gpl --enable-libx264 --prefix=/usr/local
```

5. 安装opencv

进入https://opencv.org/releases/unzip, 点击Source下载并解压(这里是4.10.0版本), 重命名文件夹为opencv
下载https://github.com/opencv/opencv_contrib/tree/4.10.0并解压到opencv-4.10.0文件夹内, 重命名为opencv_contrib

```shell
cd opencv
mkdir build
cd build
sudo cmake -D CMAKE_BUILD_TYPE=Release -D OPENCV_GENERATE_PKGCONFIG=YES -D CMAKE_INSTALL_PREFIX=/usr/local/opencv4 -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules ..
sudo make -j8
sudo make install
```

6. 运行程序
安装eigen库: 
```shell
sudo apt-get install libeigen3-dev
```

git clone下来仓库之后, include/common.h的IMG_WIDTH与IMG_HEIGHT改为视频分辨率
编译运行:
```shell
cd build
sudo make -j8
./yolov5_deepsort
```

查看NPU占用的命令: 
```shell
sudo watch -n 1 cat /sys/kernel/debug/rknpu/load
```