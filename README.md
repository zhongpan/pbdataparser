# pbdataparser
pbdataparser用于动态解析protobuf序列化数据文件，只需要proto文件即可解析。

pbdataparser可用于：
* 学习protobuf序列化结构
* 检查protobuf序列化文件是否正确

# 用法
```usage: pbdataparser <pbdatafile> <protofile> <messagetype>```
  
# 编译
* conan install -if build .
* cd build
* cmake .. -G "Visual Studio 15 2017 Win64"
* cmake --build .

# example
* pbperfm_0X0CEBFE01_11640.dat是有问题的数据

pbdataparser example/pbperfm_0X0CEBFE01_11640.dat example/alarmpm.proto nepmhead
* pbperfm_0X0CEBFE01_11645.dat是正常的数据

pbdataparser example/pbperfm_0X0CEBFE01_11645.dat example/alarmpm.proto nepmhead
