# pbdataparser
pbdataparser用于动态解析pb序列化数据文件，只需要proto文件即可解析任何pb序列化数据。
pbdataparser可用于:
* 学习pb序列化结构
* 检查pb序列化文件是否正确

# 用法
usage: pbdataparser <pbdatafile> <protofile> <messagetype>
  
# 编译
conan install -if build .
cd build
cmake .. -G "Visual Studio 15 2017 Win64"

# example
pbdataparser example/pbperfm_0X0CEBFE01_11640.dat example/alarmpm.proto nepmhead
pbdataparser example/pbperfm_0X0CEBFE01_11645.dat example/alarmpm.proto nepmhead
