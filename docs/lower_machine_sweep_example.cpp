/**
 * @file lower_machine_sweep_example.cpp
 * @brief 下位机接收sweep控制指令的示例代码
 * @note 此文件为示例代码,展示如何在下位机解析sweep电机控制指令
 */

#include <Arduino.h>

// 数据包结构定义
#pragma pack(1)
typedef struct {
    uint8_t header;          // 帧头 0x5A
    double linear_x;         // 线速度 x
    double linear_y;         // 线速度 y
    double linear_z;         // 线速度 z
    double angular_x;        // 角速度 x
    double angular_y;        // 角速度 y
    double angular_z;        // 角速度 z
    uint8_t sweep_control;   // 扫地电机控制(后4位)
    uint8_t footer;          // 帧尾 0xAA
} TwistSweepPacket_t;
#pragma pack()

// 电机控制引脚定义(根据实际硬件修改)
#define MOTOR1_PIN 5
#define MOTOR2_PIN 6
#define MOTOR3_PIN 7
#define MOTOR4_PIN 8

// 接收缓冲区
uint8_t rx_buffer[50];
uint16_t rx_index = 0;
bool in_frame = false;

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);  // 与上位机通信的串口
    
    // 初始化电机控制引脚
    pinMode(MOTOR1_PIN, OUTPUT);
    pinMode(MOTOR2_PIN, OUTPUT);
    pinMode(MOTOR3_PIN, OUTPUT);
    pinMode(MOTOR4_PIN, OUTPUT);
    
    // 初始状态:所有电机关闭
    digitalWrite(MOTOR1_PIN, LOW);
    digitalWrite(MOTOR2_PIN, LOW);
    digitalWrite(MOTOR3_PIN, LOW);
    digitalWrite(MOTOR4_PIN, LOW);
    
    Serial.println("Lower machine sweep control ready!");
}

// 控制4个扫地电机
void control_sweep_motors(uint8_t sweep_byte) {
    // 提取后4位的控制信号
    bool motor1_on = (sweep_byte & 0x01) != 0;  // bit0
    bool motor2_on = (sweep_byte & 0x02) != 0;  // bit1
    bool motor3_on = (sweep_byte & 0x04) != 0;  // bit2
    bool motor4_on = (sweep_byte & 0x08) != 0;  // bit3
    
    // 控制电机
    digitalWrite(MOTOR1_PIN, motor1_on ? HIGH : LOW);
    digitalWrite(MOTOR2_PIN, motor2_on ? HIGH : LOW);
    digitalWrite(MOTOR3_PIN, motor3_on ? HIGH : LOW);
    digitalWrite(MOTOR4_PIN, motor4_on ? HIGH : LOW);
    
    // 调试输出
    Serial.printf("Sweep control: 0x%02X -> Motor1:%d Motor2:%d Motor3:%d Motor4:%d\n",
                  sweep_byte, motor1_on, motor2_on, motor3_on, motor4_on);
}

// 解析完整数据包
bool parse_packet(const uint8_t* buffer, uint16_t len) {
    if (len != 50) {
        Serial.printf("Error: Invalid packet length %d\n", len);
        return false;
    }
    
    // 检查帧头和帧尾
    if (buffer[0] != 0x5A || buffer[49] != 0xAA) {
        Serial.printf("Error: Invalid header/footer (0x%02X, 0x%02X)\n", 
                      buffer[0], buffer[49]);
        return false;
    }
    
    // 转换为结构体指针
    TwistSweepPacket_t* packet = (TwistSweepPacket_t*)buffer;
    
    // 打印接收到的数据
    Serial.println("========== Packet Received ==========");
    Serial.printf("Linear:  x=%.3f, y=%.3f, z=%.3f\n", 
                  packet->linear_x, packet->linear_y, packet->linear_z);
    Serial.printf("Angular: x=%.3f, y=%.3f, z=%.3f\n", 
                  packet->angular_x, packet->angular_y, packet->angular_z);
    Serial.printf("Sweep:   0x%02X\n", packet->sweep_control);
    
    // 控制扫地电机
    control_sweep_motors(packet->sweep_control);
    
    // 这里可以添加对twist数据的处理
    // process_twist_data(packet->linear_x, packet->angular_z);
    
    Serial.println("====================================");
    return true;
}

// 接收并处理数据包
void receive_and_process() {
    while (Serial2.available()) {
        uint8_t byte = Serial2.read();
        
        if (!in_frame) {
            // 寻找帧头
            if (byte == 0x5A) {
                in_frame = true;
                rx_buffer[0] = byte;
                rx_index = 1;
            }
        } else {
            // 接收数据
            rx_buffer[rx_index++] = byte;
            
            // 检查是否接收完整
            if (rx_index == 50) {
                // 数据包接收完成,解析
                if (rx_buffer[49] == 0xAA) {
                    parse_packet(rx_buffer, 50);
                } else {
                    Serial.println("Error: Invalid footer");
                }
                
                // 重置状态
                in_frame = false;
                rx_index = 0;
            }
            
            // 防止缓冲区溢出
            if (rx_index >= 50) {
                Serial.println("Error: Buffer overflow, resetting");
                in_frame = false;
                rx_index = 0;
            }
        }
    }
}

void loop() {
    receive_and_process();
    
    // 其他任务...
    delay(1);
}

/*
 * 高级功能示例: PWM控制电机速度
 */
void control_sweep_motors_pwm(uint8_t sweep_byte, uint8_t pwm_value = 255) {
    // 提取后4位的控制信号
    bool motor1_on = (sweep_byte & 0x01) != 0;
    bool motor2_on = (sweep_byte & 0x02) != 0;
    bool motor3_on = (sweep_byte & 0x04) != 0;
    bool motor4_on = (sweep_byte & 0x08) != 0;
    
    // 使用PWM控制电机速度
    analogWrite(MOTOR1_PIN, motor1_on ? pwm_value : 0);
    analogWrite(MOTOR2_PIN, motor2_on ? pwm_value : 0);
    analogWrite(MOTOR3_PIN, motor3_on ? pwm_value : 0);
    analogWrite(MOTOR4_PIN, motor4_on ? pwm_value : 0);
}

/*
 * 测试函数: 模拟接收不同的sweep控制指令
 */
void test_sweep_control() {
    Serial.println("\n===== Testing Sweep Control =====");
    
    // 测试1: 全开
    Serial.println("Test 1: All motors ON");
    control_sweep_motors(0x0F);
    delay(2000);
    
    // 测试2: 全关
    Serial.println("Test 2: All motors OFF");
    control_sweep_motors(0x00);
    delay(2000);
    
    // 测试3: 只开启电机1
    Serial.println("Test 3: Only motor 1 ON");
    control_sweep_motors(0x01);
    delay(2000);
    
    // 测试4: 对角线开启(电机1和3)
    Serial.println("Test 4: Diagonal motors 1&3 ON");
    control_sweep_motors(0x05);
    delay(2000);
    
    // 测试5: 另一对角线(电机2和4)
    Serial.println("Test 5: Diagonal motors 2&4 ON");
    control_sweep_motors(0x0A);
    delay(2000);
    
    // 恢复关闭状态
    control_sweep_motors(0x00);
    Serial.println("===== Test Complete =====\n");
}
