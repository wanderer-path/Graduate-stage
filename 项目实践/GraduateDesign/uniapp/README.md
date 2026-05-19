# 物联网监控App使用说明

## 项目简介

这是一个基于UniApp开发的物联网设备监控应用，通过阿里云物联网平台API获取STM32+ESP8266上传的传感器数据，并实现远程控制功能。

## 功能特性

- 实时显示设备在线状态
- 显示温度、湿度、光强、烟雾传感器数据
- 远程控制LED开关
- 自动定时刷新数据（30秒）
- 手动刷新数据

## 项目结构

```
iot-test/
├── utils/
│   ├── aliyun-config.js    # 阿里云配置文件（需要填写您的配置）
│   ├── aliyun-sign.js      # 阿里云API签名工具
│   └── iot-service.js      # 物联网服务模块
├── pages/
│   └── index/
│       └── index.vue       # 主页面
├── pages.json              # 页面配置
└── ...
```

## 配置步骤

### 1. 获取阿里云配置信息

登录[阿里云物联网平台控制台](https://iot.console.aliyun.com/)，获取以下信息：

| 配置项 | 获取位置 |
|--------|----------|
| AccessKey ID | 阿里云控制台 → 右上角头像 → AccessKey管理 |
| AccessKey Secret | 同上 |
| RegionId | 物联网平台控制台 → 实例概览（如：cn-shanghai） |
| ProductKey | 设备管理 → 产品 → 产品详情 |
| DeviceName | 设备管理 → 设备 → 设备列表 |
| IotInstanceId | 实例概览（企业版需要，公共实例留空） |

### 2. 修改配置文件

编辑 `utils/aliyun-config.js` 文件，填入您的配置：

```javascript
export default {
    // 【必填】阿里云 AccessKey ID
    accessKeyId: 'YOUR_ACCESS_KEY_ID',
    
    // 【必填】阿里云 AccessKey Secret
    accessKeySecret: 'YOUR_ACCESS_KEY_SECRET',
    
    // 【必填】物联网平台所在地域
    regionId: 'cn-shanghai',
    
    // 【必填】产品Key
    productKey: 'YOUR_PRODUCT_KEY',
    
    // 【必填】设备名称
    deviceName: 'YOUR_DEVICE_NAME',
    
    // 【选填】实例ID（企业版需要）
    iotInstanceId: '',
    
    // ...其他配置保持默认
}
```

### 3. 配置物模型属性名称

根据您在阿里云物联网平台定义的物模型，修改 `pages/index/index.vue` 中的属性名称映射：

```javascript
// 在 refreshData() 方法中修改属性名
// 温度属性名（与物模型Identifier一致）
if (props.Temperature) { ... }

// 湿度属性名
if (props.Humidity) { ... }

// 光强属性名
if (props.LightIntensity) { ... }

// 烟雾属性名
if (props.Smoke) { ... }

// LED属性名
if (props.LED) { ... }
```

常见的物模型属性Identifier：

| 传感器 | 常见Identifier |
|--------|----------------|
| 温度 | Temperature, temperature, temp |
| 湿度 | Humidity, humidity, humi |
| 光强 | LightIntensity, light, illumination |
| 烟雾 | Smoke, smoke, MQ2 |
| LED | LED, led, LightSwitch |

## 启动项目

### 方式一：HBuilderX运行

1. 使用 HBuilderX 打开项目目录
2. 点击菜单：运行 → 运行到浏览器 → Chrome（或其他浏览器）
3. 或者：运行 → 运行到手机或模拟器

### 方式二：命令行运行

```bash
# 进入项目目录
cd iot-test

# 安装依赖（如果需要）
npm install

# H5运行
npm run dev:h5

# 微信小程序运行
npm run dev:mp-weixin

# App运行（需要HBuilderX）
# 在HBuilderX中运行
```

## 阿里云物模型配置参考

在阿里云物联网平台创建产品时，建议定义以下物模型属性：

### 传感器属性（只读）

| 标识符 | 名称 | 数据类型 | 取值范围 | 单位 |
|--------|------|----------|----------|------|
| Temperature | 温度 | float | -40~80 | °C |
| Humidity | 湿度 | float | 0~100 | % |
| LightIntensity | 光强 | int | 0~65535 | lux |
| Smoke | 烟雾浓度 | int | 0~1000 | ppm |

### 控制属性（读写）

| 标识符 | 名称 | 数据类型 | 取值范围 | 说明 |
|--------|------|----------|----------|------|
| LED | LED开关 | int | 0-关, 1-开 | 控制LED灯 |

## STM32端数据上报格式

STM32通过ESP8266向阿里云上报数据时，使用物模型属性上报格式：

```json
{
    "method": "thing.event.property.post",
    "params": {
        "Temperature": 25.5,
        "Humidity": 60,
        "LightIntensity": 500,
        "Smoke": 100,
        "LED": 0
    }
}
```

## 注意事项

1. **安全提醒**：AccessKey是敏感信息，请勿将配置文件提交到公开代码仓库
2. **跨域问题**：在H5端运行时可能遇到跨域问题，建议使用App端或小程序端测试
3. **API调用频率**：阿里云API有调用频率限制，默认30秒刷新一次
4. **网络要求**：确保设备能够访问阿里云服务器

## 常见问题

### Q: 提示"API错误"
A: 检查AccessKey、ProductKey、DeviceName是否正确配置

### Q: 数据不更新
A: 检查STM32设备是否正常上报数据，可在阿里云控制台查看设备日志

### Q: LED控制无响应
A: 确保设备在线，且物模型中LED属性定义为可写

### Q: 跨域错误
A: H5端存在跨域限制，建议使用App或小程序端，或配置代理服务器

## API参考

项目使用以下阿里云物联网平台API：

- `GetDeviceStatus` - 获取设备在线状态
- `QueryDevicePropertyStatus` - 获取设备属性
- `SetDeviceProperty` - 设置设备属性
- `QueryDeviceDetail` - 获取设备详情
- `QueryDevicePropertyData` - 获取属性历史数据

详细API文档：[阿里云物联网平台API参考](https://help.aliyun.com/document_detail/69893.html)
