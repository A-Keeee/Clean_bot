# Sweep订阅者功能说明

## 功能概述
添加了一个新的ROS2订阅者,用于订阅上位机发送的 `/sweep` 话题,控制4个扫地电机的独立开关。

## 实现细节

### 1. 订阅话题
- **话题名称**: `/sweep`
- **消息类型**: `std_msgs/msg/UInt8`
- **数据格式**: 使用后4位(bit0-bit3)分别控制4个电机
  - **bit0 (0x01)**: 电机1开关 (0=关, 1=开)
  - **bit1 (0x02)**: 电机2开关 (0=关, 1=开)
  - **bit2 (0x04)**: 电机3开关 (0=关, 1=开)
  - **bit3 (0x08)**: 电机4开关 (0=关, 1=开)
  - **bit4-bit7**: 保留(自动清零)

### 2. 控制示例
```
0x00 (0b0000): 所有电机关闭
0x01 (0b0001): 仅电机1开启
0x03 (0b0011): 电机1和2开启
0x0F (0b1111): 所有电机开启
0x05 (0b0101): 电机1和3开启
0x0A (0b1010): 电机2和4开启
```

### 2. 数据传输协议
通过Serial2将sweep控制指令发送到下位机,数据包格式如下:

```
帧头: 0x5A
线速度 linear.x (8字节 double)
线速度 linear.y (8字节 double)
线速度 linear.z (8字节 double)
角速度 angular.x (8字节 double)
角速度 angular.y (8字节 double)
角速度 angular.z (8字节 double)
扫地控制 sweep (1字节 uint8, 后4位有效)
帧尾: 0xAA
```

总长度: 50字节 (1 + 48 + 1 = 50)

### 3. 代码修改位置

#### 3.1 头文件 (fishbot.h)
- 添加了 `#include <std_msgs/msg/u_int8.h>` 头文件

#### 3.2 全局变量 (fishbot.cpp)
```cpp
std_msgs__msg__UInt8 sweep_msg;      // 扫地功能控制指令(后4位分别控制4个电机)
rcl_subscription_t sweep_subscriber; // 用于订阅扫地功能控制指令（Sweep）
```

#### 3.3 回调函数
```cpp
void callback_sweep_subscription_(const void *msgin)
```
接收并处理 `/sweep` 话题消息:
- 将接收到的值与0x0F进行AND操作,只保留后4位
- 存储到 `sweep_msg.data` 中
- 打印每个电机的状态(0或1)

#### 3.4 初始化和清理
- `create_fishbot_transport()`: 初始化sweep订阅者并添加到executor
- `destory_fishbot_transport()`: 清理sweep订阅者资源
- `setup_fishbot()`: 初始化sweep_msg默认值为0

#### 3.5 数据发送
在 `callback_twist_subscription_()` 中,将sweep值(uint8)作为数据包的最后一个字节(在帧尾0xAA之前)发送。

## 使用方法

### 上位机发送命令
```bash
# 开启所有4个电机
ros2 topic pub /sweep std_msgs/msg/UInt8 "{data: 15}"  # 0x0F = 0b1111

# 只开启电机1
ros2 topic pub /sweep std_msgs/msg/UInt8 "{data: 1}"   # 0x01 = 0b0001

# 开启电机1和电机3
ros2 topic pub /sweep std_msgs/msg/UInt8 "{data: 5}"   # 0x05 = 0b0101

# 开启电机2和电机4
ros2 topic pub /sweep std_msgs/msg/UInt8 "{data: 10}"  # 0x0A = 0b1010

# 关闭所有电机
ros2 topic pub /sweep std_msgs/msg/UInt8 "{data: 0}"   # 0x00 = 0b0000
```

### 下位机接收处理
下位机需要按照50字节的数据包格式解析数据:
1. 检查帧头是否为 0x5A
2. 读取48字节的twist数据
3. 读取1字节的sweep控制指令
4. 检查帧尾是否为 0xAA
5. 解析sweep字节的后4位:
   ```c
   uint8_t motor1_on = (sweep_byte & 0x01) ? 1 : 0;  // bit0
   uint8_t motor2_on = (sweep_byte & 0x02) ? 1 : 0;  // bit1
   uint8_t motor3_on = (sweep_byte & 0x04) ? 1 : 0;  // bit2
   uint8_t motor4_on = (sweep_byte & 0x08) ? 1 : 0;  // bit3
   ```

## 注意事项
1. sweep值的高4位会被自动清零,只保留后4位
2. 默认值为0 (所有电机关闭)
3. 每次发送twist消息时都会附带当前的sweep状态
4. executor的容量已从3增加到4,以支持新增的订阅者
5. 每个bit独立控制一个电机,可以实现任意组合

## 测试建议
1. 发布不同的sweep值,观察下位机4个电机的状态是否正确
2. 测试边界值和组合值:
   - 0x00: 全关
   - 0x0F: 全开
   - 0x05: 对角开启
   - 0x0A: 另一对角开启
3. 同时发送twist和sweep消息,确认数据包完整性
4. 测试大于15的值,确认高4位被正确清零
