#!/bin/bash

# 下载 tinyxml_2_6_2.zip 文件
wget https://udomain.dl.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip

# 解压缩文件
unzip tinyxml_2_6_2.zip
rm tinyxml_2_6_2.zip

# 进入 tinyxml 目录
cd tinyxml

# 修改 makefile 中的 OUTPUT 值为 libtinyxml.a
sed -i 's/OUTPUT := xmltest/OUTPUT := libtinyxml.a/' Makefile

# 修改编译规则
sed -i 's/${OUTPUT}: ${OBJS}/${OUTPUT}: ${OBJS}\n\t${AR} $@ ${OBJS}\n\t${RANLIB} $@/' Makefile
sed -i 's/${LD} -o $@ ${LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}//' Makefile

# 编译并安装
make

# 创建目录 tinyxml
mkdir -p tinyxml

# 复制头文件到 tinyxml 目录
cp *.h tinyxml/

# 复制静态库到 tinyxml 目录
cp libtinyxml.a tinyxml/

# 移动 tinyxml 目录到 /usr/local
sudo mv tinyxml /usr/local/