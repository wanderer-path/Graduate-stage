/**
 * 阿里云物联网平台服务模块
 * 封装设备状态查询、属性获取、属性设置等功能
 */

import config from './aliyun-config.js'
import { buildRequestUrl } from './aliyun-sign.js'

/**
 * 发起HTTP请求到阿里云API
 * @param {string} action - API操作名称
 * @param {Object} params - 业务参数
 * @returns {Promise} - 请求结果
 */
function request(action, params = {}) {
	return new Promise((resolve, reject) => {
		const url = buildRequestUrl(action, params)
		
		uni.request({
			url: url,
			method: 'GET',
			success: (res) => {
				if (res.statusCode === 200) {
					// 检查阿里云API返回的错误
					if (res.data.Code && res.data.Code !== '200') {
						reject(new Error(`API错误: ${res.data.Code} - ${res.data.Message}`))
					} else {
						resolve(res.data)
					}
				} else {
					reject(new Error(`HTTP错误: ${res.statusCode}`))
				}
			},
			fail: (err) => {
				reject(new Error(`请求失败: ${err.errMsg}`))
			}
		})
	})
}

/**
 * 获取设备在线状态
 * @param {string} productKey - 产品Key（可选，默认使用配置文件中的值）
 * @param {string} deviceName - 设备名称（可选，默认使用配置文件中的值）
 * @returns {Promise<Object>} - 设备状态信息
 *   - Status: 设备状态 (ONLINE:在线, OFFLINE:离线, UNACTIVE:未激活)
 */
export async function getDeviceStatus(productKey = config.productKey, deviceName = config.deviceName) {
	const params = {
		ProductKey: productKey,
		DeviceName: deviceName
	}
	
	// 企业版实例需要添加实例ID
	if (config.iotInstanceId) {
		params.IotInstanceId = config.iotInstanceId
	}
	
	const result = await request('GetDeviceStatus', params)
	return result.Data
}

// /**
//  * 查询设备详细信息
//  * @param {string} productKey - 产品Key
//  * @param {string} deviceName - 设备名称
//  * @returns {Promise<Object>} - 设备详细信息
//  */
// export async function getDeviceInfo(productKey = config.productKey, deviceName = config.deviceName) {
// 	const params = {
// 		ProductKey: productKey,
// 		DeviceName: deviceName
// 	}
	
// 	if (config.iotInstanceId) {
// 		params.IotInstanceId = config.iotInstanceId
// 	}
	
// 	const result = await request('QueryDeviceDetail', params)
// 	return result.Data
// }

/**
 * 获取设备物模型属性（最新上报的属性值）
 * 用于获取温度、湿度、光强、烟雾等传感器数据
 * @param {string} productKey - 产品Key
 * @param {string} deviceName - 设备名称
 * @returns {Promise<Object>} - 设备属性列表
 */
export async function getDeviceProperties(productKey = config.productKey, deviceName = config.deviceName) {
	const params = {
		ProductKey: productKey,
		DeviceName: deviceName
	}
	
	if (config.iotInstanceId) {
		params.IotInstanceId = config.iotInstanceId
	}
	
	const result = await request('QueryDevicePropertyStatus', params)
	
	// 将属性列表转换为更易用的对象格式
	// 返回格式: { propertyName: { value: xxx, time: xxx }, ... }
	const properties = {}
	if (result.Data && result.Data.List && result.Data.List.PropertyStatusInfo) {
		result.Data.List.PropertyStatusInfo.forEach(prop => {
			properties[prop.Identifier] = {
				value: prop.Value,
				time: prop.Time,
				name: prop.Name || prop.Identifier
			}
		})
	}
	
	return properties
}

/**
 * 设置设备属性（下发控制命令）
 * 用于控制LED开关等
 * @param {Object} propertyData - 要设置的属性，格式: { propertyName: value }
 * @param {string} productKey - 产品Key
 * @param {string} deviceName - 设备名称
 * @returns {Promise<Object>} - 设置结果
 * 
 * 示例: setDeviceProperty({ LED: 1 }) // 打开LED
 *       setDeviceProperty({ LED: 0 }) // 关闭LED
 */
export async function setDeviceProperty(items, productKey = config.productKey, deviceName = config.deviceName) {
	const params = {
		ProductKey: productKey,
		DeviceName: deviceName,
		// Items参数需要是JSON字符串格式
		Items: items
	}
	
	if (config.iotInstanceId) {
		params.IotInstanceId = config.iotInstanceId
	}
	
	const result = await request('SetDeviceProperty', params)
	return result
}

// /**
//  * 调用设备服务（用于更复杂的设备控制）
//  * @param {string} serviceName - 服务名称（物模型中定义的服务Identifier）
//  * @param {Object} serviceArgs - 服务参数
//  * @param {string} productKey - 产品Key
//  * @param {string} deviceName - 设备名称
//  * @returns {Promise<Object>} - 调用结果
//  */
// export async function invokeDeviceService(serviceName, serviceArgs = {}, productKey = config.productKey, deviceName = config.deviceName) {
// 	const params = {
// 		ProductKey: productKey,
// 		DeviceName: deviceName,
// 		Identifier: serviceName,
// 		Args: JSON.stringify(serviceArgs)
// 	}
	
// 	if (config.iotInstanceId) {
// 		params.IotInstanceId = config.iotInstanceId
// 	}
	
// 	const result = await request('InvokeThingService', params)
// 	return result
// }

// /**
//  * 批量获取设备属性历史数据
//  * @param {string} identifier - 属性标识符（如: Temperature, Humidity等）
//  * @param {number} startTime - 开始时间戳（毫秒）
//  * @param {number} endTime - 结束时间戳（毫秒）
//  * @param {number} pageSize - 每页数量，默认10
//  * @param {string} productKey - 产品Key
//  * @param {string} deviceName - 设备名称
//  * @returns {Promise<Array>} - 历史数据列表
//  */
// export async function getPropertyHistory(identifier, startTime, endTime, pageSize = 10, productKey = config.productKey, deviceName = config.deviceName) {
// 	const params = {
// 		ProductKey: productKey,
// 		DeviceName: deviceName,
// 		Identifier: identifier,
// 		StartTime: startTime,
// 		EndTime: endTime,
// 		PageSize: pageSize,
// 		Asc: 0 // 倒序排列，最新的在前
// 	}
	
// 	if (config.iotInstanceId) {
// 		params.IotInstanceId = config.iotInstanceId
// 	}
	
// 	const result = await request('QueryDevicePropertyData', params)
// 	return result.Data && result.Data.List ? result.Data.List.PropertyInfo : []
// }

/**
 * 综合获取设备所有数据（状态 + 属性）
 * 便捷方法，一次性获取设备的在线状态和所有属性
 * @param {string} productKey - 产品Key
 * @param {string} deviceName - 设备名称
 * @returns {Promise<Object>} - 完整的设备数据
 */
export async function getDeviceData(productKey = config.productKey, deviceName = config.deviceName) {
	try {
		// 并行获取状态和属性，提高效率
		const [statusResult, propertiesResult] = await Promise.all([
			getDeviceStatus(productKey, deviceName),
			getDeviceProperties(productKey, deviceName)
		])
		
		return {
			// 设备在线状态: ONLINE/OFFLINE/UNACTIVE
			online: statusResult.Status === 'ONLINE',
			status: statusResult.Status,
			// 设备属性数据
			properties: propertiesResult,
			// 获取时间
			updateTime: new Date().toLocaleString()
		}
	} catch (error) {
		console.error('获取设备数据失败:', error)
		throw error
	}
}

// 导出默认配置，方便页面获取配置信息
export { config }
