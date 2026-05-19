/**
 * 阿里云API签名工具
 * 使用经过验证的加密算法实现
 */

import config from './aliyun-config.js'

/**
 * UTF-8编码字符串转字节数组
 */
function utf8ToBytes(str) {
	const bytes = []
	for (let i = 0; i < str.length; i++) {
		let c = str.charCodeAt(i)
		if (c < 0x80) {
			bytes.push(c)
		} else if (c < 0x800) {
			bytes.push(0xc0 | (c >> 6))
			bytes.push(0x80 | (c & 0x3f))
		} else if (c < 0xd800 || c >= 0xe000) {
			bytes.push(0xe0 | (c >> 12))
			bytes.push(0x80 | ((c >> 6) & 0x3f))
			bytes.push(0x80 | (c & 0x3f))
		} else {
			i++
			c = 0x10000 + (((c & 0x3ff) << 10) | (str.charCodeAt(i) & 0x3ff))
			bytes.push(0xf0 | (c >> 18))
			bytes.push(0x80 | ((c >> 12) & 0x3f))
			bytes.push(0x80 | ((c >> 6) & 0x3f))
			bytes.push(0x80 | (c & 0x3f))
		}
	}
	return bytes
}

/**
 * 字节数组转Base64
 */
function bytesToBase64(bytes) {
	const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
	let result = ''
	for (let i = 0; i < bytes.length; i += 3) {
		const b1 = bytes[i]
		const b2 = i + 1 < bytes.length ? bytes[i + 1] : 0
		const b3 = i + 2 < bytes.length ? bytes[i + 2] : 0
		result += chars[b1 >> 2]
		result += chars[((b1 & 3) << 4) | (b2 >> 4)]
		result += i + 1 < bytes.length ? chars[((b2 & 15) << 2) | (b3 >> 6)] : '='
		result += i + 2 < bytes.length ? chars[b3 & 63] : '='
	}
	return result
}

/**
 * SHA1算法实现
 */
function sha1(message) {
	const bytes = typeof message === 'string' ? utf8ToBytes(message) : message.slice()
	
	// 初始化
	let h0 = 0x67452301
	let h1 = 0xEFCDAB89
	let h2 = 0x98BADCFE
	let h3 = 0x10325476
	let h4 = 0xC3D2E1F0
	
	// 预处理：添加填充位
	const msgLen = bytes.length
	const bitLenHi = Math.floor(msgLen / 0x20000000)  // 高32位
	const bitLenLo = msgLen << 3                       // 低32位
	
	bytes.push(0x80)
	while ((bytes.length % 64) !== 56) {
		bytes.push(0)
	}
	
	// 添加64位长度（大端序）
	bytes.push((bitLenHi >>> 24) & 0xFF)
	bytes.push((bitLenHi >>> 16) & 0xFF)
	bytes.push((bitLenHi >>> 8) & 0xFF)
	bytes.push(bitLenHi & 0xFF)
	bytes.push((bitLenLo >>> 24) & 0xFF)
	bytes.push((bitLenLo >>> 16) & 0xFF)
	bytes.push((bitLenLo >>> 8) & 0xFF)
	bytes.push(bitLenLo & 0xFF)
	
	// 处理每个512位块
	for (let i = 0; i < bytes.length; i += 64) {
		const w = new Array(80)
		
		for (let j = 0; j < 16; j++) {
			w[j] = ((bytes[i + j * 4] << 24) |
					(bytes[i + j * 4 + 1] << 16) |
					(bytes[i + j * 4 + 2] << 8) |
					bytes[i + j * 4 + 3]) >>> 0
		}
		
		for (let j = 16; j < 80; j++) {
			const n = (w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16]) >>> 0
			w[j] = ((n << 1) | (n >>> 31)) >>> 0
		}
		
		let a = h0, b = h1, c = h2, d = h3, e = h4
		
		for (let j = 0; j < 80; j++) {
			let f, k
			if (j < 20) {
				f = ((b & c) | ((~b >>> 0) & d)) >>> 0
				k = 0x5A827999
			} else if (j < 40) {
				f = (b ^ c ^ d) >>> 0
				k = 0x6ED9EBA1
			} else if (j < 60) {
				f = ((b & c) | (b & d) | (c & d)) >>> 0
				k = 0x8F1BBCDC
			} else {
				f = (b ^ c ^ d) >>> 0
				k = 0xCA62C1D6
			}
			
			const temp = (((a << 5) | (a >>> 27)) + f + e + k + w[j]) >>> 0
			e = d
			d = c
			c = ((b << 30) | (b >>> 2)) >>> 0
			b = a
			a = temp
		}
		
		h0 = (h0 + a) >>> 0
		h1 = (h1 + b) >>> 0
		h2 = (h2 + c) >>> 0
		h3 = (h3 + d) >>> 0
		h4 = (h4 + e) >>> 0
	}
	
	// 输出20字节结果
	const result = []
	for (const h of [h0, h1, h2, h3, h4]) {
		result.push((h >>> 24) & 0xFF)
		result.push((h >>> 16) & 0xFF)
		result.push((h >>> 8) & 0xFF)
		result.push(h & 0xFF)
	}
	return result
}

/**
 * HMAC-SHA1算法实现
 */
function hmacSha1(key, message) {
	const blockSize = 64
	let keyBytes = typeof key === 'string' ? utf8ToBytes(key) : key.slice()
	
	// 如果key长度超过blockSize，先进行SHA1
	if (keyBytes.length > blockSize) {
		keyBytes = sha1(keyBytes)
	}
	
	// 补齐key到blockSize
	while (keyBytes.length < blockSize) {
		keyBytes.push(0)
	}
	
	// 计算内外padding
	const ipad = keyBytes.map(b => b ^ 0x36)
	const opad = keyBytes.map(b => b ^ 0x5C)
	
	// 内层hash: SHA1(ipad + message)
	const msgBytes = typeof message === 'string' ? utf8ToBytes(message) : message
	const innerData = [...ipad, ...msgBytes]
	const innerHash = sha1(innerData)
	
	// 外层hash: SHA1(opad + innerHash)
	const outerData = [...opad, ...innerHash]
	return sha1(outerData)
}

/**
 * 生成UUID
 */
function generateUUID() {
	return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
		const r = Math.random() * 16 | 0
		const v = c === 'x' ? r : (r & 0x3 | 0x8)
		return v.toString(16)
	})
}

/**
 * 特殊URL编码（阿里云要求的编码方式）
 */
function specialUrlEncode(str) {
	return encodeURIComponent(String(str))
		.replace(/\+/g, '%2B')
		.replace(/\*/g, '%2A')
		.replace(/%7E/g, '~')
}

/**
 * 生成ISO8601格式的UTC时间戳
 */
function getTimestamp() {
	return new Date().toISOString().replace(/\.\d{3}/, '')
}

/**
 * 构建阿里云API签名
 */
function buildSignature(params) {
	// 1. 按参数名ASCII排序
	const sortedKeys = Object.keys(params).sort()
	
	// 2. 构建规范化查询字符串
	const canonicalizedQueryString = sortedKeys
		.map(key => `${specialUrlEncode(key)}=${specialUrlEncode(params[key])}`)
		.join('&')
	
	// 3. 构建待签名字符串
	// StringToSign = HTTPMethod + "&" + percentEncode("/") + "&" + percentEncode(CanonicalizedQueryString)
	const stringToSign = 'GET&' + specialUrlEncode('/') + '&' + specialUrlEncode(canonicalizedQueryString)
	
	// 调试：打印待签名字符串（可注释掉）
	// console.log('StringToSign:', stringToSign)
	
	// 4. 计算签名（签名密钥 = AccessKeySecret + "&"）
	const signKey = config.accessKeySecret + '&'
	const signatureBytes = hmacSha1(signKey, stringToSign)
	const signature = bytesToBase64(signatureBytes)
	
	// console.log('Signature:', signature)
	
	return signature
}

/**
 * 构建完整的API请求参数（包含公共参数和签名）
 */
export function buildRequestParams(action, businessParams = {}) {
	// 公共参数
	const commonParams = {
		Format: 'JSON',
		Version: config.apiVersion,
		AccessKeyId: config.accessKeyId,
		SignatureMethod: 'HMAC-SHA1',
		Timestamp: getTimestamp(),
		SignatureVersion: '1.0',
		SignatureNonce: generateUUID(),
		Action: action,
		RegionId: config.regionId
	}
	
	// 合并参数
	const allParams = { ...commonParams, ...businessParams }
	
	// 计算签名
	allParams.Signature = buildSignature(allParams)
	
	return allParams
}

/**
 * 构建完整的请求URL
 */
export function buildRequestUrl(action, businessParams = {}) {
	const params = buildRequestParams(action, businessParams)
	const queryString = Object.keys(params)
		.map(key => `${encodeURIComponent(key)}=${encodeURIComponent(params[key])}`)
		.join('&')
	
	return `https://${config.endpoint}/?${queryString}`
}
