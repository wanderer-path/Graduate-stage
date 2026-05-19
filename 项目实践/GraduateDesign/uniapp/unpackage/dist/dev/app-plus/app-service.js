if (typeof Promise !== "undefined" && !Promise.prototype.finally) {
  Promise.prototype.finally = function(callback) {
    const promise = this.constructor;
    return this.then(
      (value) => promise.resolve(callback()).then(() => value),
      (reason) => promise.resolve(callback()).then(() => {
        throw reason;
      })
    );
  };
}
;
if (typeof uni !== "undefined" && uni && uni.requireGlobal) {
  const global = uni.requireGlobal();
  ArrayBuffer = global.ArrayBuffer;
  Int8Array = global.Int8Array;
  Uint8Array = global.Uint8Array;
  Uint8ClampedArray = global.Uint8ClampedArray;
  Int16Array = global.Int16Array;
  Uint16Array = global.Uint16Array;
  Int32Array = global.Int32Array;
  Uint32Array = global.Uint32Array;
  Float32Array = global.Float32Array;
  Float64Array = global.Float64Array;
  BigInt64Array = global.BigInt64Array;
  BigUint64Array = global.BigUint64Array;
}
;
if (uni.restoreGlobal) {
  uni.restoreGlobal(Vue, weex, plus, setTimeout, clearTimeout, setInterval, clearInterval);
}
(function(vue) {
  "use strict";
  function formatAppLog(type, filename, ...args) {
    if (uni.__log__) {
      uni.__log__(type, filename, ...args);
    } else {
      console[type].apply(console, [...args, filename]);
    }
  }
  const config = {
    // ==================== 必填配置 ====================
    // 阿里云 AccessKey ID（在阿里云控制台 -> AccessKey管理 中获取）
    accessKeyId: "LTAI5tSkLjiD8MmAecJu1stZ",
    // 阿里云 AccessKey Secret（在阿里云控制台 -> AccessKey管理 中获取）
    accessKeySecret: "ith1UNq0Rh8SBuiajPZz8p2OrXxGbI",
    // 物联网平台所在地域ID（如：cn-shanghai）
    // 常用地域：cn-shanghai(华东2)、cn-beijing(华北2)、cn-shenzhen(华南1)
    regionId: "cn-shanghai",
    // 产品的ProductKey（在物联网平台控制台 -> 设备管理 -> 产品 中获取）
    productKey: "k0shgVICsVV",
    // 设备的DeviceName（在物联网平台控制台 -> 设备管理 -> 设备 中获取）
    deviceName: "stm32f103c8t6",
    // 实例ID（企业版实例需要填写，公共实例可以留空）
    // 在物联网平台控制台 -> 实例概览 中获取
    iotInstanceId: "iot-06z00i8refyslm1",
    // ==================== API端点配置 ====================
    // 物联网平台API端点（根据地域自动生成，一般不需要修改）
    // 格式: iot.{regionId}.aliyuncs.com
    get endpoint() {
      return `iot.${this.regionId}.aliyuncs.com`;
    },
    // API版本号（固定值，不需要修改）
    apiVersion: "2018-01-20"
  };
  function utf8ToBytes(str) {
    const bytes = [];
    for (let i = 0; i < str.length; i++) {
      let c = str.charCodeAt(i);
      if (c < 128) {
        bytes.push(c);
      } else if (c < 2048) {
        bytes.push(192 | c >> 6);
        bytes.push(128 | c & 63);
      } else if (c < 55296 || c >= 57344) {
        bytes.push(224 | c >> 12);
        bytes.push(128 | c >> 6 & 63);
        bytes.push(128 | c & 63);
      } else {
        i++;
        c = 65536 + ((c & 1023) << 10 | str.charCodeAt(i) & 1023);
        bytes.push(240 | c >> 18);
        bytes.push(128 | c >> 12 & 63);
        bytes.push(128 | c >> 6 & 63);
        bytes.push(128 | c & 63);
      }
    }
    return bytes;
  }
  function bytesToBase64(bytes) {
    const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let result = "";
    for (let i = 0; i < bytes.length; i += 3) {
      const b1 = bytes[i];
      const b2 = i + 1 < bytes.length ? bytes[i + 1] : 0;
      const b3 = i + 2 < bytes.length ? bytes[i + 2] : 0;
      result += chars[b1 >> 2];
      result += chars[(b1 & 3) << 4 | b2 >> 4];
      result += i + 1 < bytes.length ? chars[(b2 & 15) << 2 | b3 >> 6] : "=";
      result += i + 2 < bytes.length ? chars[b3 & 63] : "=";
    }
    return result;
  }
  function sha1(message) {
    const bytes = typeof message === "string" ? utf8ToBytes(message) : message.slice();
    let h0 = 1732584193;
    let h1 = 4023233417;
    let h2 = 2562383102;
    let h3 = 271733878;
    let h4 = 3285377520;
    const msgLen = bytes.length;
    const bitLenHi = Math.floor(msgLen / 536870912);
    const bitLenLo = msgLen << 3;
    bytes.push(128);
    while (bytes.length % 64 !== 56) {
      bytes.push(0);
    }
    bytes.push(bitLenHi >>> 24 & 255);
    bytes.push(bitLenHi >>> 16 & 255);
    bytes.push(bitLenHi >>> 8 & 255);
    bytes.push(bitLenHi & 255);
    bytes.push(bitLenLo >>> 24 & 255);
    bytes.push(bitLenLo >>> 16 & 255);
    bytes.push(bitLenLo >>> 8 & 255);
    bytes.push(bitLenLo & 255);
    for (let i = 0; i < bytes.length; i += 64) {
      const w = new Array(80);
      for (let j = 0; j < 16; j++) {
        w[j] = (bytes[i + j * 4] << 24 | bytes[i + j * 4 + 1] << 16 | bytes[i + j * 4 + 2] << 8 | bytes[i + j * 4 + 3]) >>> 0;
      }
      for (let j = 16; j < 80; j++) {
        const n = (w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16]) >>> 0;
        w[j] = (n << 1 | n >>> 31) >>> 0;
      }
      let a = h0, b = h1, c = h2, d = h3, e = h4;
      for (let j = 0; j < 80; j++) {
        let f, k;
        if (j < 20) {
          f = (b & c | ~b >>> 0 & d) >>> 0;
          k = 1518500249;
        } else if (j < 40) {
          f = (b ^ c ^ d) >>> 0;
          k = 1859775393;
        } else if (j < 60) {
          f = (b & c | b & d | c & d) >>> 0;
          k = 2400959708;
        } else {
          f = (b ^ c ^ d) >>> 0;
          k = 3395469782;
        }
        const temp = (a << 5 | a >>> 27) + f + e + k + w[j] >>> 0;
        e = d;
        d = c;
        c = (b << 30 | b >>> 2) >>> 0;
        b = a;
        a = temp;
      }
      h0 = h0 + a >>> 0;
      h1 = h1 + b >>> 0;
      h2 = h2 + c >>> 0;
      h3 = h3 + d >>> 0;
      h4 = h4 + e >>> 0;
    }
    const result = [];
    for (const h of [h0, h1, h2, h3, h4]) {
      result.push(h >>> 24 & 255);
      result.push(h >>> 16 & 255);
      result.push(h >>> 8 & 255);
      result.push(h & 255);
    }
    return result;
  }
  function hmacSha1(key, message) {
    const blockSize = 64;
    let keyBytes = typeof key === "string" ? utf8ToBytes(key) : key.slice();
    if (keyBytes.length > blockSize) {
      keyBytes = sha1(keyBytes);
    }
    while (keyBytes.length < blockSize) {
      keyBytes.push(0);
    }
    const ipad = keyBytes.map((b) => b ^ 54);
    const opad = keyBytes.map((b) => b ^ 92);
    const msgBytes = typeof message === "string" ? utf8ToBytes(message) : message;
    const innerData = [...ipad, ...msgBytes];
    const innerHash = sha1(innerData);
    const outerData = [...opad, ...innerHash];
    return sha1(outerData);
  }
  function generateUUID() {
    return "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function(c) {
      const r = Math.random() * 16 | 0;
      const v = c === "x" ? r : r & 3 | 8;
      return v.toString(16);
    });
  }
  function specialUrlEncode(str) {
    return encodeURIComponent(String(str)).replace(/\+/g, "%2B").replace(/\*/g, "%2A").replace(/%7E/g, "~");
  }
  function getTimestamp() {
    return (/* @__PURE__ */ new Date()).toISOString().replace(/\.\d{3}/, "");
  }
  function buildSignature(params) {
    const sortedKeys = Object.keys(params).sort();
    const canonicalizedQueryString = sortedKeys.map((key) => `${specialUrlEncode(key)}=${specialUrlEncode(params[key])}`).join("&");
    const stringToSign = "GET&" + specialUrlEncode("/") + "&" + specialUrlEncode(canonicalizedQueryString);
    const signKey = config.accessKeySecret + "&";
    const signatureBytes = hmacSha1(signKey, stringToSign);
    const signature = bytesToBase64(signatureBytes);
    return signature;
  }
  function buildRequestParams(action, businessParams = {}) {
    const commonParams = {
      Format: "JSON",
      Version: config.apiVersion,
      AccessKeyId: config.accessKeyId,
      SignatureMethod: "HMAC-SHA1",
      Timestamp: getTimestamp(),
      SignatureVersion: "1.0",
      SignatureNonce: generateUUID(),
      Action: action,
      RegionId: config.regionId
    };
    const allParams = { ...commonParams, ...businessParams };
    allParams.Signature = buildSignature(allParams);
    return allParams;
  }
  function buildRequestUrl(action, businessParams = {}) {
    const params = buildRequestParams(action, businessParams);
    const queryString = Object.keys(params).map((key) => `${encodeURIComponent(key)}=${encodeURIComponent(params[key])}`).join("&");
    return `https://${config.endpoint}/?${queryString}`;
  }
  function request(action, params = {}) {
    return new Promise((resolve, reject) => {
      const url = buildRequestUrl(action, params);
      uni.request({
        url,
        method: "GET",
        success: (res) => {
          if (res.statusCode === 200) {
            if (res.data.Code && res.data.Code !== "200") {
              reject(new Error(`API错误: ${res.data.Code} - ${res.data.Message}`));
            } else {
              resolve(res.data);
            }
          } else {
            reject(new Error(`HTTP错误: ${res.statusCode}`));
          }
        },
        fail: (err) => {
          reject(new Error(`请求失败: ${err.errMsg}`));
        }
      });
    });
  }
  async function getDeviceStatus(productKey = config.productKey, deviceName = config.deviceName) {
    const params = {
      ProductKey: productKey,
      DeviceName: deviceName
    };
    {
      params.IotInstanceId = config.iotInstanceId;
    }
    const result = await request("GetDeviceStatus", params);
    return result.Data;
  }
  async function getDeviceProperties(productKey = config.productKey, deviceName = config.deviceName) {
    const params = {
      ProductKey: productKey,
      DeviceName: deviceName
    };
    {
      params.IotInstanceId = config.iotInstanceId;
    }
    const result = await request("QueryDevicePropertyStatus", params);
    const properties = {};
    if (result.Data && result.Data.List && result.Data.List.PropertyStatusInfo) {
      result.Data.List.PropertyStatusInfo.forEach((prop) => {
        properties[prop.Identifier] = {
          value: prop.Value,
          time: prop.Time,
          name: prop.Name || prop.Identifier
        };
      });
    }
    return properties;
  }
  async function setDeviceProperty(items, productKey = config.productKey, deviceName = config.deviceName) {
    const params = {
      ProductKey: productKey,
      DeviceName: deviceName,
      // Items参数需要是JSON字符串格式
      Items: items
    };
    {
      params.IotInstanceId = config.iotInstanceId;
    }
    const result = await request("SetDeviceProperty", params);
    return result;
  }
  async function getDeviceData(productKey = config.productKey, deviceName = config.deviceName) {
    try {
      const [statusResult, propertiesResult] = await Promise.all([
        getDeviceStatus(productKey, deviceName),
        getDeviceProperties(productKey, deviceName)
      ]);
      return {
        // 设备在线状态: ONLINE/OFFLINE/UNACTIVE
        online: statusResult.Status === "ONLINE",
        status: statusResult.Status,
        // 设备属性数据
        properties: propertiesResult,
        // 获取时间
        updateTime: (/* @__PURE__ */ new Date()).toLocaleString()
      };
    } catch (error) {
      formatAppLog("error", "at utils/iot-service.js:223", "获取设备数据失败:", error);
      throw error;
    }
  }
  const _export_sfc = (sfc, props) => {
    const target = sfc.__vccOpts || sfc;
    for (const [key, val] of props) {
      target[key] = val;
    }
    return target;
  };
  const _sfc_main$1 = {
    data() {
      return {
        // 设备在线状态
        deviceOnline: false,
        // 加载状态
        loading: false,
        // 错误信息
        errorMsg: "",
        // 传感器数据
        sensorData: {
          temperature: "--",
          // 温度
          humidity: "--",
          // 湿度
          light: "--",
          // 光强
          smoke: "--"
          // 烟雾
        },
        // 控制设备数据
        led_switch: false,
        // LED状态
        servo_angle: false,
        // 门窗状态
        beep_switch: false,
        // 蜂鸣器状态
        // LD6002C毫米波雷达检测信息
        person_exist: false,
        fall_status: false,
        // 定时刷新器
        refreshTimer: null
      };
    },
    onLoad() {
      this.refreshData();
      this.refreshTimer = setInterval(() => {
        this.refreshData();
      }, 1e3);
    },
    onUnload() {
      if (this.refreshTimer) {
        clearInterval(this.refreshTimer);
      }
    },
    methods: {
      /**
       * 刷新设备数据
       * 获取设备在线状态和所有传感器属性
       */
      async refreshData() {
        this.errorMsg = "";
        try {
          const data = await getDeviceData();
          this.deviceOnline = data.online;
          const props = data.properties;
          if (props.temperature) {
            this.sensorData.temperature = props.temperature.value;
          }
          if (props.humidity) {
            this.sensorData.humidity = props.humidity.value;
          }
          if (props.light) {
            this.sensorData.light = props.light.value;
          }
          if (props.smoke) {
            this.sensorData.smoke = props.smoke.value;
          }
          if (props.led_switch) {
            if (props.led_switch.value === "1") {
              this.led_switch = true;
            } else if (props.led_switch.value === "0") {
              this.led_switch = false;
            }
          }
          if (props.servo_angle) {
            this.servo_angle = props.servo_angle.value;
            if (props.servo_angle.value === "1") {
              this.servo_angle = true;
            } else if (props.servo_angle.value === "0") {
              this.servo_angle = false;
            }
          }
          if (props.beep_switch) {
            this.beep_switch = props.beep_switch.value;
            if (props.beep_switch.value === "1") {
              this.beep_switch = true;
            } else if (props.beep_switch.value === "0") {
              this.beep_switch = false;
            }
          }
          if (props.person_exist) {
            this.person_exist = props.person_exist.value;
            if (props.person_exist.value === "1") {
              this.person_exist = true;
            } else if (props.person_exist.value === "0") {
              this.person_exist = false;
            }
          }
          if (props.fall_status) {
            this.fall_status = props.fall_status.value;
            if (props.fall_status.value === "1") {
              this.fall_status = true;
              uni.showModal({
                title: "提示",
                content: "检测到用户可能跌倒，请及时处理",
                success(res) {
                  if (res.confirm) {
                    formatAppLog("log", "at pages/index/index.vue:288", "用户点击了确认");
                  } else if (res.cancel) {
                    formatAppLog("log", "at pages/index/index.vue:290", "用户点击了取消");
                  }
                }
              });
            } else if (props.fall_status.value === "0") {
              this.fall_status = false;
            }
          }
          formatAppLog("log", "at pages/index/index.vue:299", "设备数据:", data);
        } catch (error) {
          formatAppLog("error", "at pages/index/index.vue:302", "获取数据失败:", error);
          this.errorMsg = error.message || "获取数据失败";
          setTimeout(() => {
            this.errorMsg = "";
          }, 3e3);
        }
      },
      /**
       * 切换 LIGHT 状态
       */
      async toggleLight() {
        const LIGHT_SWITCH_ON = '{"led_switch":1}';
        const LIGHT_SWITCH_OFF = '{"led_switch":0}';
        const newStatus = !this.led_switch;
        this.loading = true;
        this.errorMsg = "";
        try {
          await setDeviceProperty(newStatus ? LIGHT_SWITCH_ON : LIGHT_SWITCH_OFF);
          this.led_switch = newStatus;
          uni.showToast({
            title: newStatus ? "灯光已开启" : "灯光已关闭",
            icon: "success"
          });
        } catch (error) {
          formatAppLog("error", "at pages/index/index.vue:336", "控制LED失败:", error);
          this.errorMsg = error.message || "控制失败";
          this.led_switch = !newStatus;
          setTimeout(() => {
            this.errorMsg = "";
          }, 3e3);
        } finally {
          this.loading = false;
        }
      },
      /**
       * 切换 WINDOW 状态
       */
      async toggleWindow() {
        const WINDOW_SWITCH_ON = '{"servo_angle":1}';
        const WINDOW_SWITCH_OFF = '{"servo_angle":0}';
        const newStatus = !this.servo_angle;
        this.loading = true;
        this.errorMsg = "";
        try {
          await setDeviceProperty(newStatus ? WINDOW_SWITCH_ON : WINDOW_SWITCH_OFF);
          this.servo_angle = newStatus;
          uni.showToast({
            title: newStatus ? "门窗已开启" : "门窗已关闭",
            icon: "success"
          });
        } catch (error) {
          formatAppLog("error", "at pages/index/index.vue:375", "控制门窗失败:", error);
          this.errorMsg = error.message || "控制失败";
          this.servo_angle = !newStatus;
          setTimeout(() => {
            this.errorMsg = "";
          }, 3e3);
        } finally {
          this.loading = false;
        }
      },
      /**
       * 切换 BEEP 状态
       */
      async toggleBeep() {
        const BEEP_SWITCH_ON = '{"beep_switch":1}';
        const BEEP_SWITCH_OFF = '{"beep_switch":0}';
        const newStatus = !this.beep_switch;
        this.loading = true;
        this.errorMsg = "";
        try {
          await setDeviceProperty(newStatus ? BEEP_SWITCH_ON : BEEP_SWITCH_OFF);
          this.beep_switch = newStatus;
          uni.showToast({
            title: newStatus ? "蜂鸣器已开启" : "蜂鸣器已关闭",
            icon: "success"
          });
        } catch (error) {
          formatAppLog("error", "at pages/index/index.vue:414", "控制蜂鸣器失败:", error);
          this.errorMsg = error.message || "控制失败";
          this.beep_switch = !newStatus;
          setTimeout(() => {
            this.errorMsg = "";
          }, 3e3);
        } finally {
          this.loading = false;
        }
      }
    }
  };
  function _sfc_render(_ctx, _cache, $props, $setup, $data, $options) {
    return vue.openBlock(), vue.createElementBlock("view", { class: "container" }, [
      vue.createElementVNode("view", { class: "header" }, [
        vue.createElementVNode("view", { class: "device-info" }, [
          vue.createElementVNode("text", { class: "device-name" }, "智能环境监测设备"),
          vue.createElementVNode(
            "view",
            {
              class: vue.normalizeClass(["status-badge", $data.deviceOnline ? "online" : "offline"])
            },
            [
              vue.createElementVNode("view", { class: "status-dot" }),
              vue.createElementVNode(
                "text",
                null,
                vue.toDisplayString($data.deviceOnline ? "在线" : "离线"),
                1
                /* TEXT */
              )
            ],
            2
            /* CLASS */
          )
        ])
      ]),
      vue.createElementVNode("view", { class: "sensor-grid" }, [
        vue.createElementVNode("view", { class: "sensor-row" }, [
          vue.createElementVNode("view", { class: "sensor-card" }, [
            vue.createElementVNode("view", { class: "sensor-icon temp-icon" }, "🌡️"),
            vue.createElementVNode("view", { class: "sensor-info" }, [
              vue.createElementVNode("text", { class: "sensor-label" }, "温度"),
              vue.createElementVNode("text", { class: "sensor-value" }, [
                vue.createTextVNode(
                  vue.toDisplayString($data.sensorData.temperature),
                  1
                  /* TEXT */
                ),
                vue.createElementVNode("text", { class: "sensor-unit" }, "°C")
              ])
            ])
          ]),
          vue.createElementVNode("view", { class: "sensor-card" }, [
            vue.createElementVNode("view", { class: "sensor-icon humid-icon" }, "💧"),
            vue.createElementVNode("view", { class: "sensor-info" }, [
              vue.createElementVNode("text", { class: "sensor-label" }, "湿度"),
              vue.createElementVNode("text", { class: "sensor-value" }, [
                vue.createTextVNode(
                  vue.toDisplayString($data.sensorData.humidity),
                  1
                  /* TEXT */
                ),
                vue.createElementVNode("text", { class: "sensor-unit" }, "%")
              ])
            ])
          ])
        ]),
        vue.createElementVNode("view", { class: "sensor-row" }, [
          vue.createElementVNode("view", { class: "sensor-card" }, [
            vue.createElementVNode("view", { class: "sensor-icon light-icon" }, "☀️"),
            vue.createElementVNode("view", { class: "sensor-info" }, [
              vue.createElementVNode("text", { class: "sensor-label" }, "光强"),
              vue.createElementVNode("text", { class: "sensor-value" }, [
                vue.createTextVNode(
                  vue.toDisplayString($data.sensorData.light),
                  1
                  /* TEXT */
                ),
                vue.createElementVNode("text", { class: "sensor-unit" }, "lux")
              ])
            ])
          ]),
          vue.createElementVNode("view", { class: "sensor-card" }, [
            vue.createElementVNode("view", { class: "sensor-icon smoke-icon" }, "🔥"),
            vue.createElementVNode("view", { class: "sensor-info" }, [
              vue.createElementVNode("text", { class: "sensor-label" }, "烟雾"),
              vue.createElementVNode("text", { class: "sensor-value" }, [
                vue.createTextVNode(
                  vue.toDisplayString($data.sensorData.smoke),
                  1
                  /* TEXT */
                ),
                vue.createElementVNode("text", { class: "sensor-unit" }, "ppm")
              ])
            ])
          ])
        ])
      ]),
      vue.createElementVNode("view", { class: "control-section" }, [
        vue.createElementVNode("text", { class: "section-title" }, "设备控制"),
        vue.createElementVNode("view", { class: "control-card" }, [
          vue.createElementVNode("view", { class: "control-left" }, [
            vue.createElementVNode(
              "view",
              {
                class: vue.normalizeClass(["light-icon", $data.led_switch ? "light-on" : "light-off"])
              },
              vue.toDisplayString($data.led_switch ? "💡" : "🔦"),
              3
              /* TEXT, CLASS */
            ),
            vue.createElementVNode("view", { class: "control-info" }, [
              vue.createElementVNode("text", { class: "control-label" }, "LED灯"),
              vue.createElementVNode(
                "text",
                { class: "control-status" },
                vue.toDisplayString($data.led_switch ? "已开启" : "已关闭"),
                1
                /* TEXT */
              )
            ])
          ]),
          vue.createElementVNode("switch", {
            checked: $data.led_switch,
            onClick: _cache[0] || (_cache[0] = (...args) => $options.toggleLight && $options.toggleLight(...args)),
            disabled: $data.loading
          }, null, 8, ["checked", "disabled"])
        ]),
        vue.createElementVNode("view", { class: "control-card" }, [
          vue.createElementVNode("view", { class: "control-left" }, [
            vue.createElementVNode(
              "view",
              {
                class: vue.normalizeClass(["window-icon", $data.servo_angle ? "window-on" : "window-off"])
              },
              vue.toDisplayString($data.servo_angle ? "🔓" : "🔒"),
              3
              /* TEXT, CLASS */
            ),
            vue.createElementVNode("view", { class: "control-info" }, [
              vue.createElementVNode("text", { class: "control-label" }, "门窗"),
              vue.createElementVNode(
                "text",
                { class: "control-status" },
                vue.toDisplayString($data.servo_angle ? "已开启" : "已关闭"),
                1
                /* TEXT */
              )
            ])
          ]),
          vue.createElementVNode("switch", {
            checked: $data.servo_angle,
            onClick: _cache[1] || (_cache[1] = (...args) => $options.toggleWindow && $options.toggleWindow(...args)),
            disabled: $data.loading
          }, null, 8, ["checked", "disabled"])
        ]),
        vue.createElementVNode("view", { class: "control-card" }, [
          vue.createElementVNode("view", { class: "control-left" }, [
            vue.createElementVNode(
              "view",
              {
                class: vue.normalizeClass(["beep-icon", $data.beep_switch ? "beep-on" : "beep-off"])
              },
              vue.toDisplayString($data.beep_switch ? "🔔" : "🔕"),
              3
              /* TEXT, CLASS */
            ),
            vue.createElementVNode("view", { class: "control-info" }, [
              vue.createElementVNode("text", { class: "control-label" }, "蜂鸣器"),
              vue.createElementVNode(
                "text",
                { class: "control-status" },
                vue.toDisplayString($data.beep_switch ? "已开启" : "已关闭"),
                1
                /* TEXT */
              )
            ])
          ]),
          vue.createElementVNode("switch", {
            checked: $data.beep_switch,
            onClick: _cache[2] || (_cache[2] = (...args) => $options.toggleBeep && $options.toggleBeep(...args)),
            disabled: $data.loading
          }, null, 8, ["checked", "disabled"])
        ])
      ]),
      vue.createElementVNode("view", { class: "detection-section" }, [
        vue.createElementVNode("text", { class: "section-title" }, "人体检测状态"),
        vue.createElementVNode("view", { class: "detection-grid" }, [
          vue.createElementVNode("view", { class: "detection-card" }, [
            vue.createElementVNode(
              "view",
              {
                class: vue.normalizeClass(["detection-icon", $data.person_exist ? "person-detected" : "person-none"])
              },
              vue.toDisplayString($data.person_exist ? "🚶" : "👤"),
              3
              /* TEXT, CLASS */
            ),
            vue.createElementVNode("view", { class: "detection-info" }, [
              vue.createElementVNode("text", { class: "detection-label" }, "人体存在"),
              vue.createElementVNode(
                "text",
                {
                  class: vue.normalizeClass(["detection-status", $data.person_exist ? "status-active" : "status-inactive"])
                },
                vue.toDisplayString($data.person_exist ? "有人存在" : "无人存在"),
                3
                /* TEXT, CLASS */
              )
            ])
          ]),
          vue.createElementVNode("view", { class: "detection-card" }, [
            vue.createElementVNode(
              "view",
              {
                class: vue.normalizeClass(["detection-icon", $data.fall_status ? "fall-detected" : "fall-none"])
              },
              vue.toDisplayString($data.fall_status ? "⚠️" : "✅"),
              3
              /* TEXT, CLASS */
            ),
            vue.createElementVNode("view", { class: "detection-info" }, [
              vue.createElementVNode("text", { class: "detection-label" }, "跌倒检测"),
              vue.createElementVNode(
                "text",
                {
                  class: vue.normalizeClass(["detection-status", $data.fall_status ? "status-danger" : "status-safe"])
                },
                vue.toDisplayString($data.fall_status ? "发生跌倒" : "状态正常"),
                3
                /* TEXT, CLASS */
              )
            ])
          ])
        ])
      ]),
      $data.errorMsg ? (vue.openBlock(), vue.createElementBlock("view", {
        key: 0,
        class: "error-toast"
      }, [
        vue.createElementVNode(
          "text",
          null,
          vue.toDisplayString($data.errorMsg),
          1
          /* TEXT */
        )
      ])) : vue.createCommentVNode("v-if", true)
    ]);
  }
  const PagesIndexIndex = /* @__PURE__ */ _export_sfc(_sfc_main$1, [["render", _sfc_render], ["__file", "E:/Trae/GradateDesign/pages/index/index.vue"]]);
  __definePage("pages/index/index", PagesIndexIndex);
  const _sfc_main = {
    onLaunch: function() {
      formatAppLog("log", "at App.vue:4", "App Launch");
    },
    onShow: function() {
      formatAppLog("log", "at App.vue:7", "App Show");
    },
    onHide: function() {
      formatAppLog("log", "at App.vue:10", "App Hide");
    }
  };
  const App = /* @__PURE__ */ _export_sfc(_sfc_main, [["__file", "E:/Trae/GradateDesign/App.vue"]]);
  function createApp() {
    const app = vue.createVueApp(App);
    return {
      app
    };
  }
  const { app: __app__, Vuex: __Vuex__, Pinia: __Pinia__ } = createApp();
  uni.Vuex = __Vuex__;
  uni.Pinia = __Pinia__;
  __app__.provide("__globalStyles", __uniConfig.styles);
  __app__._component.mpType = "app";
  __app__._component.render = () => {
  };
  __app__.mount("#app");
})(Vue);
