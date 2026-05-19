<template>
	<view class="container">
		<!-- 头部：设备状态 -->
		<view class="header">
			<view class="device-info">
				<text class="device-name">智能环境监测设备</text>
				<view class="status-badge" :class="deviceOnline ? 'online' : 'offline'">
					<view class="status-dot"></view>
					<text>{{ deviceOnline ? '在线' : '离线' }}</text>
				</view>
			</view>
		</view>

		<!-- 传感器数据展示区域 -->
		<view class="sensor-grid">
			<view class="sensor-row">
				<!-- 温度 -->
				<view class="sensor-card">
					<view class="sensor-icon temp-icon">🌡️</view>
					<view class="sensor-info">
						<text class="sensor-label">温度</text>
						<text class="sensor-value">{{ sensorData.temperature }}<text
								class="sensor-unit">°C</text></text>
					</view>
				</view>

				<!-- 湿度 -->
				<view class="sensor-card">
					<view class="sensor-icon humid-icon">💧</view>
					<view class="sensor-info">
						<text class="sensor-label">湿度</text>
						<text class="sensor-value">{{ sensorData.humidity }}<text class="sensor-unit">%</text></text>
					</view>
				</view>
			</view>
			<view class="sensor-row">
				<!-- 光强 -->
				<view class="sensor-card">
					<view class="sensor-icon light-icon">☀️</view>
					<view class="sensor-info">
						<text class="sensor-label">光强</text>
						<text class="sensor-value">{{ sensorData.light }}<text class="sensor-unit">lux</text></text>
					</view>
				</view>

				<!-- 烟雾 -->
				<view class="sensor-card">
					<view class="sensor-icon smoke-icon">🔥</view>
					<view class="sensor-info">
						<text class="sensor-label">烟雾</text>
						<text class="sensor-value">{{ sensorData.smoke }}<text class="sensor-unit">ppm</text></text>
					</view>
				</view>
			</view>
		</view>

		<!-- 设备控制区域 -->
		<view class="control-section">
			<text class="section-title">设备控制</text>

			<!-- LED控制 -->
			<view class="control-card">
				<view class="control-left">
					<!-- 使用图标表示灯泡状态 -->
					<view class="light-icon" :class="led_switch ? 'light-on' : 'light-off'">
						{{ led_switch ? '💡' : '🔦' }}
					</view>
					<view class="control-info">
						<text class="control-label">LED灯</text>
						<text class="control-status">{{ led_switch ? '已开启' : '已关闭' }}</text>
					</view>
				</view>
				<switch :checked="led_switch" @click="toggleLight" :disabled="loading" />
			</view>

			<!-- 门窗控制 -->
			<view class="control-card">
				<view class="control-left">
					<!-- 使用图标表示门窗状态 -->
					<view class="window-icon" :class="servo_angle ? 'window-on' : 'window-off'">
						{{ servo_angle ? '🔓' : '🔒' }}
					</view>
					<view class="control-info">
						<text class="control-label">门窗</text>
						<text class="control-status">{{ servo_angle ? '已开启' : '已关闭' }}</text>
					</view>
				</view>
				<switch :checked="servo_angle" @click="toggleWindow" :disabled="loading" />
			</view>

			<!-- 蜂鸣器控制 -->
				<view class="control-card">
					<view class="control-left">
						<!-- 使用图标表示蜂鸣器状态 -->
						<view class="beep-icon" :class="beep_switch ? 'beep-on' : 'beep-off'">
							{{ beep_switch ? '🔔' : '🔕' }}
						</view>
						<view class="control-info">
							<text class="control-label">蜂鸣器</text>
							<text class="control-status">{{ beep_switch ? '已开启' : '已关闭' }}</text>
						</view>
					</view>
					<switch :checked="beep_switch" @click="toggleBeep" :disabled="loading" />
				</view>
			</view>

		<!-- 人体检测状态区域 -->
		<view class="detection-section">
			<text class="section-title">人体检测状态</text>
			<view class="detection-grid">
				<!-- 人体存在状态 -->
				<view class="detection-card">
					<view class="detection-icon" :class="person_exist ? 'person-detected' : 'person-none'">
						{{ person_exist ? '🚶' : '👤' }}
					</view>
					<view class="detection-info">
						<text class="detection-label">人体存在</text>
						<text class="detection-status" :class="person_exist ? 'status-active' : 'status-inactive'">
							{{ person_exist ? '有人存在' : '无人存在' }}
						</text>
					</view>
				</view>
				<!-- 跌倒状态 -->
				<view class="detection-card">
					<view class="detection-icon" :class="fall_status ? 'fall-detected' : 'fall-none'">
						{{ fall_status ? '⚠️' : '✅' }}
					</view>
					<view class="detection-info">
						<text class="detection-label">跌倒检测</text>
						<text class="detection-status" :class="fall_status ? 'status-danger' : 'status-safe'">
							{{ fall_status ? '发生跌倒' : '状态正常' }}
						</text>
					</view>
				</view>
			</view>
		</view>

		<!-- 错误提示 -->
		<view class="error-toast" v-if="errorMsg">
			<text>{{ errorMsg }}</text>
		</view>
	</view>
</template>

<script>
/**
 * 物联网设备监控主页面
 * 功能：显示设备状态、传感器数据、控制LED
 */
import { getDeviceData, setDeviceProperty } from '@/utils/iot-service.js'

export default {
	data() {
		return {
			// 设备在线状态
			deviceOnline: false,
			// 加载状态
			loading: false,
			// 错误信息
			errorMsg: '',
			// 传感器数据
			sensorData: {
				temperature: '--',  // 温度
				humidity: '--',     // 湿度
				light: '--',        // 光强
				smoke: '--'         // 烟雾
			},
			// 控制设备数据
			led_switch: false,		// LED状态
			servo_angle: false,	// 门窗状态
			beep_switch: false,		// 蜂鸣器状态
			// LD6002C毫米波雷达检测信息
			person_exist: false,
			fall_status: false,
			// 定时刷新器
			refreshTimer: null
		}
	},

	onLoad() {
		// 页面加载时获取数据
		this.refreshData()

		// 设置定时刷新（每5秒刷新一次）
		this.refreshTimer = setInterval(() => {
			this.refreshData()
		}, 5000)
	},

	onUnload() {
		// 页面卸载时清除定时器
		if (this.refreshTimer) {
			clearInterval(this.refreshTimer)
		}
	},

	methods: {
		/**
		 * 刷新设备数据
		 * 获取设备在线状态和所有传感器属性
		 */
		async refreshData() {
			this.errorMsg = ''

			try {
				// 调用服务获取设备数据
				const data = await getDeviceData()

				// 更新设备在线状态
				this.deviceOnline = data.online

				// 解析传感器数据
				// 注意：属性名称需要与您在阿里云物模型中定义的Identifier一致
				// 以下是常见的属性名，请根据实际情况修改
				const props = data.properties

				// 温度 - 常见属性名: temperature
				if (props.temperature) {
					this.sensorData.temperature = props.temperature.value
				}

				// 湿度 - 常见属性名: humidity
				if (props.humidity) {
					this.sensorData.humidity = props.humidity.value
				}

				// 光强 - 常见属性名:  light
				if (props.light) {
					this.sensorData.light = props.light.value
				}

				// 烟雾 - 常见属性名: smoke
				if (props.smoke) {
					this.sensorData.smoke = props.smoke.value
				}

				// LED状态 - 常见属性名: led_switch
				if (props.led_switch) {
					if (props.led_switch.value === '1') {	// 1表示开启，0表示关闭
						this.led_switch = true
					} else if (props.led_switch.value === '0') {
						this.led_switch = false
					}
				}

				// 门窗状态 - 常见属性名: servo_angle
				if (props.servo_angle) {
					this.servo_angle = props.servo_angle.value
					if (props.servo_angle.value === '1') { // 1表示开启，0表示关闭
						this.servo_angle = true
					} else if (props.servo_angle.value === '0') {
						this.servo_angle = false
					}
				}

				// 蜂鸣器状态 - 常见属性名: beep_switch
				if (props.beep_switch) {
					this.beep_switch = props.beep_switch.value
					if (props.beep_switch.value === '1') { // 1表示开启，0表示关闭
						this.beep_switch = true
					} else if (props.beep_switch.value === '0') {
						this.beep_switch = false
					}
				}

				// 人体存在状态 - 常见属性名: person_exist
				if (props.person_exist) {
					this.person_exist = props.person_exist.value
					if (props.person_exist.value === '1') { // 1表示开启，0表示关闭
						this.person_exist = true
					} else if (props.person_exist.value === '0') {
						this.person_exist = false
					}
				}
				
				// 跌倒状态 - 常见属性名: fall_status
				if (props.fall_status) {
					this.fall_status = props.fall_status.value
					if (props.fall_status.value === '1') { // 1表示开启，0表示关闭
						this.fall_status = true
						// 手机震动
						// 提示用户跌倒，并且需要确认
						uni.showModal({
							title: '提示',
							content: '检测到用户可能跌倒，请及时处理',
							success(res) {
								if (res.confirm) {
								console.log('用户点击了确认')
								} else if (res.cancel) {
								console.log('用户点击了取消')
								}
							}
						})
					} else if (props.fall_status.value === '0') {
						this.fall_status = false
					}
				}

				console.log('设备数据:', data)

			} catch (error) {
				console.error('获取数据失败:', error)
				this.errorMsg = error.message || '获取数据失败'

				// 3秒后清除错误提示
				setTimeout(() => {
					this.errorMsg = ''
				}, 3000)
			}
		},

		/**
		 * 切换 LIGHT 状态
		 */
		async toggleLight() {
			const LIGHT_SWITCH_ON = '{"led_switch":1}'
			const LIGHT_SWITCH_OFF = '{"led_switch":0}'

			const newStatus = !this.led_switch;

			this.loading = true
			this.errorMsg = ''

			try {
				// 下发属性设置命令led_switch
				await setDeviceProperty(newStatus ? LIGHT_SWITCH_ON : LIGHT_SWITCH_OFF)

				this.led_switch = newStatus

				uni.showToast({
					title: newStatus ? '灯光已开启' : '灯光已关闭',
					icon: 'success'
				})

			} catch (error) {
				console.error('控制LED失败:', error)
				this.errorMsg = error.message || '控制失败'

				// 恢复开关状态（因为下发失败）
				this.led_switch = !newStatus

				setTimeout(() => {
					this.errorMsg = ''
				}, 3000)
			} finally {
				this.loading = false
			}
		},

		/**
		 * 切换 WINDOW 状态
		 */
		async toggleWindow() {
			const WINDOW_SWITCH_ON = '{"servo_angle":1}'
			const WINDOW_SWITCH_OFF = '{"servo_angle":0}'

			const newStatus = !this.servo_angle;

			this.loading = true
			this.errorMsg = ''

			try {
				// 下发属性设置命令servo_angle
				await setDeviceProperty(newStatus ? WINDOW_SWITCH_ON : WINDOW_SWITCH_OFF)

				// 更新本地状态
				this.servo_angle = newStatus

				uni.showToast({
					title: newStatus ? '门窗已开启' : '门窗已关闭',
					icon: 'success'
				})

			} catch (error) {
				console.error('控制门窗失败:', error)
				this.errorMsg = error.message || '控制失败'

				// 恢复开关状态（因为下发失败）
				this.servo_angle = !newStatus

				setTimeout(() => {
					this.errorMsg = ''
				}, 3000)
			} finally {
				this.loading = false
			}
		},

		/**
		 * 切换 BEEP 状态
		 */
		async toggleBeep() {
			const BEEP_SWITCH_ON = '{"beep_switch":1}'
			const BEEP_SWITCH_OFF = '{"beep_switch":0}'

			const newStatus = !this.beep_switch;

			this.loading = true
			this.errorMsg = ''

			try {
				// 下发属性设置命令beep_switch
				await setDeviceProperty(newStatus ? BEEP_SWITCH_ON : BEEP_SWITCH_OFF)

				// 更新本地状态
				this.beep_switch = newStatus
				
				uni.showToast({
					title: newStatus ? '蜂鸣器已开启' : '蜂鸣器已关闭',
					icon: 'success'
				})

			} catch (error) {
				console.error('控制蜂鸣器失败:', error)
				this.errorMsg = error.message || '控制失败'

				// 恢复开关状态（因为下发失败）
				this.beep_switch = !newStatus

				setTimeout(() => {
					this.errorMsg = ''
				}, 3000)
			} finally {
				this.loading = false
			}
		},

	}
}
</script>

<style>
.container {
	min-height: 100vh;
	background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
	padding: 30rpx;
}

/* 头部样式 */
.header {
	background: rgba(255, 255, 255, 0.95);
	border-radius: 20rpx;
	padding: 30rpx;
	margin-bottom: 30rpx;
	box-shadow: 0 4rpx 20rpx rgba(0, 0, 0, 0.1);
}

.device-info {
	display: flex;
	justify-content: space-between;
	align-items: center;
	margin-bottom: 10rpx;
}

.device-name {
	font-size: 36rpx;
	font-weight: bold;
	color: #333;
}

.status-badge {
	display: flex;
	align-items: center;
	padding: 8rpx 20rpx;
	border-radius: 30rpx;
	font-size: 24rpx;
}

.status-badge.online {
	background: #e8f5e9;
	color: #4caf50;
}

.status-badge.offline {
	background: #ffebee;
	color: #f44336;
}

.status-dot {
	width: 16rpx;
	height: 16rpx;
	border-radius: 50%;
	margin-right: 10rpx;
}

.online .status-dot {
	background: #4caf50;
}

.offline .status-dot {
	background: #f44336;
}

/* 传感器卡片网格 */
.sensor-grid {
    display: flex;
    flex-direction: column;
    margin-bottom: 30rpx;
}

.sensor-row {
    display: flex;
    justify-content: space-between;
    margin-bottom: 20rpx;
}

.sensor-card {
	width: 48%;
	background: rgba(255, 255, 255, 0.95);
	border-radius: 20rpx;
	padding: 30rpx;
	margin: 5rpx 5rpx;
	display: flex;
	align-items: center;
	box-shadow: 0 4rpx 20rpx rgba(0, 0, 0, 0.1);
}

.sensor-icon {
	font-size: 60rpx;
	margin-right: 20rpx;
}

.sensor-info {
	display: flex;
	flex-direction: column;
}

.sensor-label {
	font-size: 26rpx;
	color: #666;
	margin-bottom: 8rpx;
}

.sensor-value {
	font-size: 44rpx;
	font-weight: bold;
	color: #333;
}

.sensor-unit {
	font-size: 24rpx;
	color: #999;
	margin-left: 4rpx;
}

/* 控制区域 */
.control-section {
	background: rgba(255, 255, 255, 0.95);
	border-radius: 20rpx;
	padding: 30rpx;
	margin-bottom: 30rpx;
	box-shadow: 0 4rpx 20rpx rgba(0, 0, 0, 0.1);
}

.section-title {
	font-size: 32rpx;
	font-weight: bold;
	color: #333;
	margin-bottom: 20rpx;
	display: block;
}

.control-card {
	display: flex;
	justify-content: space-between;
	align-items: center;
	padding: 20rpx 0;
	border-bottom: 1rpx solid #eee;
}

.control-card:last-child {
	border-bottom: none;
}

.control-left {
	display: flex;
	align-items: center;
}

.light-icon {
	font-size: 60rpx;
	margin-right: 20rpx;
	transition: all 0.3s;
}

.light-on {
	filter: drop-shadow(0 0 10rpx #ffd700);
}

.light-off {
	opacity: 0.6;
}

.window-icon {
	font-size: 60rpx;
	margin-right: 20rpx;
	transition: all 0.3s;
}

.window-on {
	filter: drop-shadow(0 0 10rpx #1eb241);
}

.window-off {
	opacity: 0.6;
}

.beep-icon {
	font-size: 60rpx;
	margin-right: 20rpx;
	transition: all 0.3s;
}

.beep-on {
	filter: drop-shadow(0 0 10rpx #c21d1d);
}

.beep-off {
	opacity: 0.6;
}

.control-info {
	display: flex;
	flex-direction: column;
}

.control-label {
	font-size: 30rpx;
	color: #333;
	margin-bottom: 6rpx;
}

.control-status {
	font-size: 24rpx;
	color: #999;
}

/* 人体检测状态区域 */
.detection-section {
	background: rgba(255, 255, 255, 0.95);
	border-radius: 20rpx;
	padding: 30rpx;
	margin-bottom: 30rpx;
	box-shadow: 0 4rpx 20rpx rgba(0, 0, 0, 0.1);
}

.detection-grid {
	display: flex;
	justify-content: space-between;
}

.detection-card {
	width: 48%;
	background: #f8f9fa;
	border-radius: 16rpx;
	padding: 24rpx;
	display: flex;
	align-items: center;
	box-shadow: 0 2rpx 10rpx rgba(0, 0, 0, 0.05);
}

.detection-icon {
	font-size: 56rpx;
	margin-right: 16rpx;
}

.person-detected {
	filter: drop-shadow(0 0 10rpx #4caf50);
}

.person-none {
	opacity: 0.5;
}

.fall-detected {
	filter: drop-shadow(0 0 10rpx #f44336);
	animation: pulse 1s infinite;
}

.fall-none {
	opacity: 0.6;
}

@keyframes pulse {
	0%, 100% { transform: scale(1); }
	50% { transform: scale(1.1); }
}

.detection-info {
	display: flex;
	flex-direction: column;
}

.detection-label {
	font-size: 26rpx;
	color: #666;
	margin-bottom: 6rpx;
}

.detection-status {
	font-size: 28rpx;
	font-weight: bold;
}

.status-active {
	color: #4caf50;
}

.status-inactive {
	color: #999;
}

.status-danger {
	color: #f44336;
}

.status-safe {
	color: #4caf50;
}

/* 错误提示 */
.error-toast {
	position: fixed;
	bottom: 200rpx;
	left: 50%;
	transform: translateX(-50%);
	background: rgba(244, 67, 54, 0.9);
	color: #fff;
	padding: 20rpx 40rpx;
	border-radius: 10rpx;
	font-size: 28rpx;
	z-index: 999;
}
</style>
