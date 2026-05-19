/**
 * 阿里云物联网平台配置文件
 * 请根据您的阿里云账号信息填写以下配置
 */

export default {
	// ==================== 必填配置 ====================
	
	// 阿里云 AccessKey ID（在阿里云控制台 -> AccessKey管理 中获取）
	accessKeyId: 'LTAI5tSkLjiD8MmAecJu1stZ',
	
	// 阿里云 AccessKey Secret（在阿里云控制台 -> AccessKey管理 中获取）
	accessKeySecret: 'ith1UNq0Rh8SBuiajPZz8p2OrXxGbI',
	
	// 物联网平台所在地域ID（如：cn-shanghai）
	// 常用地域：cn-shanghai(华东2)、cn-beijing(华北2)、cn-shenzhen(华南1)
	regionId: 'cn-shanghai',
	
	// 产品的ProductKey（在物联网平台控制台 -> 设备管理 -> 产品 中获取）
	productKey: 'k0shgVICsVV',
	
	// 设备的DeviceName（在物联网平台控制台 -> 设备管理 -> 设备 中获取）
	deviceName: 'stm32f103c8t6',
	
	// 实例ID（企业版实例需要填写，公共实例可以留空）
	// 在物联网平台控制台 -> 实例概览 中获取
	iotInstanceId: 'iot-06z00i8refyslm1',
	
	// ==================== API端点配置 ====================
	
	// 物联网平台API端点（根据地域自动生成，一般不需要修改）
	// 格式: iot.{regionId}.aliyuncs.com
	get endpoint() {
		return `iot.${this.regionId}.aliyuncs.com`
	},
	
	// API版本号（固定值，不需要修改）
	apiVersion: '2018-01-20'
}
