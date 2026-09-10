package com.jinyuni.dengbei_care

//import org.eclipse.paho.android.service.AlarmPingSender;
//import kotlinx.coroutines.flow.internal.NoOpContinuation.context
import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.content.pm.PackageManager
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.PowerManager
import android.util.Log
import androidx.annotation.RequiresPermission
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import com.github.mikephil.charting.data.Entry
import com.jinyuni.dengbei_care.ui.home.HomeViewModel
import com.jinyuni.dengbei_care.ui.notifications.NotificationsViewModel
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.selects.onTimeout
import kotlinx.coroutines.selects.select
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import org.eclipse.paho.android.service.MqttAndroidClient
import org.eclipse.paho.android.service.MqttService
import org.eclipse.paho.client.mqttv3.IMqttActionListener
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.IMqttToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttException
import org.eclipse.paho.client.mqttv3.MqttMessage
import java.security.KeyStore
import java.security.cert.CertificateFactory
import java.util.UUID
import javax.crypto.Cipher
import javax.crypto.spec.SecretKeySpec
import javax.net.ssl.SSLContext
import javax.net.ssl.TrustManagerFactory

data class HumidityTemperatureData(
    val timestamp: Long,
    val temperature: Double,
    val humidity: Double
)




data class ParsedFrameRaw(
    val gatewayIdHex: String,        // 形如 "3C:1A:40:7A:4E:E0"
    val devCount: Int,
    val payloadsRaw8: List<ByteArray>// 每设备 8 字节原始有效载荷
)

data class DeviceReadingV3(
    val devIdBytes: ByteArray,   // 原始4字节（大端）
    val devIdHex: String,        // 形如 "0C:11:82:24"
    val humidityPct: Double,     // 湿度（×0.1）
    val temperatureC: Double     // 温度（×0.1），如需有符号可改成 s16be
)

data class DeviceReading(
    val devIdBytes: ByteArray,  // 4B（大端）
    val devIdHex: String,
    val humidityPct: Double,    // ×0.1 %RH
    val temperatureC: Double    // ×0.1 °C（带符号）
)
data class ParsedFrame(
    val gatewayIdHex: String,   // 6B 大端
    val devCount: Int,
    val readings: List<DeviceReading>
)


class MqtttService : MqttService() {
    private lateinit var mqttClient: MqttAndroidClient
    private val NOTIFICATION_ID = 1
    private val CHANNEL_ID = "MqttServiceChannel"
    private val serverUri = "ssl://117.72.84.210:8883"
    private val clientId = "android_client"
    private var rtopic0 = "/topic/get_0123"
    private var rtopic1 = "/topic/get_01234"
    private var stopic0 = "/topic/get_0123"
    private val serviceScope = CoroutineScope(Dispatchers.IO + Job())
    private val reconnectInterval: Long = 1000*300
    private val publishInterval = 10000L // 10 秒
    private val publishTopic = "/topic/test0"
//    private val publishMessage = "hello world"
    private lateinit var homeViewModel: HomeViewModel
    private lateinit var prefs: SharedPreferences
    private lateinit var notificationViewModel: NotificationsViewModel
    var device_mac:String = ""
    private val GW_WRITE_UUID    = UUID.fromString("0000ffe3-0000-1000-8000-00805F9B34FB")
    private val GW_SERVICE_UUID  = UUID.fromString("0000ffe0-0000-1000-8000-00805F9B34FB")


    private lateinit var readbackdata:String
    private lateinit var readbacktopic:String

    private lateinit var readBackTemperature:String
    private lateinit var readBackHuminity:String

    val APP_AES_KEY16 = byteArrayOf(
        0x91.toByte(), 0x4E.toByte(), 0x03.toByte(), 0xB7.toByte(),
        0xC2.toByte(), 0x5A.toByte(), 0x88.toByte(), 0x1D.toByte(),
        0xF4.toByte(), 0x60.toByte(), 0x7B.toByte(), 0x2E.toByte(),
        0xA9.toByte(), 0x17.toByte(), 0x6C.toByte(), 0x55.toByte()
    )

    // 单设备的历史读数 + 上次报警时间(替代老 dataA/B/C 5/10/15MinutesAgo 9 个变量 + 全局 timeAlarmBkp)
    // 多设备场景下每个设备需要独立的历史和报警节流
    data class DeviceHistory(
        var data5MinAgo: HumidityTemperatureData? = null,
        var data10MinAgo: HumidityTemperatureData? = null,
        var data15MinAgo: HumidityTemperatureData? = null,
        var lastAlarmTime: Long = 0L   // 上次触发报警的时间(毫秒),用于 900 秒节流
    )
    private val deviceHistory = mutableMapOf<String, DeviceHistory>()

    /**
     * 单设备的报警参数快照。processTemperatureHumidityData 每次调用时从
     * DeviceParamsBook 加载(没设过的字段 fallback 到全局默认值)。
     */
    data class DeviceParams(
        val tempDrop: Double,
        val humiDrop: Double,
        val tempmax: Double,
        val tempmin: Double,
        val humimax: Double,
        val humimin: Double,
        val humitempDrop: Int,         // 下降报警开关:0/1
        val thresholdBeyond: Int,       // 阈值报警开关:0/1
        val vibrationAlarmEnable: Int, // 震动报警开关:0/1
        val voiceAlarmEnable: Int,     // 蜂鸣报警开关:0/1
        val delayAlarm: Double
    )

    /** 按设备 ID 加载报警参数。优先读 dev_<devId>_<param>,未设置则 fallback 到全局 key,再无则用代码默认值。 */
    private fun loadDeviceParams(devId: String): DeviceParams {
        val ctx = applicationContext
        fun d(param: String, default: String): Double =
            DeviceParamsBook.getOrGlobal(ctx, devId, param, default).toDoubleOrNull() ?: default.toDouble()
        fun flag(param: String, default: String): Int =
            if (DeviceParamsBook.getOrGlobal(ctx, devId, param, default) == "1") 1 else 0
        return DeviceParams(
            tempDrop = d("tempdrop", "2"),
            humiDrop = d("humdrop", "1"),
            tempmax = d("tempmax", "35"),
            tempmin = d("tempmin", "29"),
            humimax = d("hummax", "75"),
            humimin = d("hummin", "40"),
            humitempDrop = flag("dengbeialarm", "1"),
            thresholdBeyond = flag("yuzhialarm", "1"),
            vibrationAlarmEnable = flag("zhendongalarm", "1"),
            voiceAlarmEnable = flag("xianglingalarm", "1"),
            delayAlarm = d("delayalarm", "8")
        )
    }



    private var temp_before15:Double = -50.0
    private var temp_before10:Double = -50.0
    private var temp_before5:Double = -50.0
    private var humi_before15:Double = -50.0
    private var humi_before10:Double = -50.0
    private var humi_before5:Double = -50.0
    //    15、10、5分钟以前的时间的记录，用来判断数据是否过期了
    private var time_before15:Long = 0
    private var time_before10:Long = 0
    private var time_before5:Long = 0
    private var currentTime:Long = 0
    //    用来判断数据有没有重复
    private var _bkp_temp_time:Long = 0
    private var _bkp_humi_timme:Long = 0
    private var _bkp_all_time:Long = 0
    //    数据过期的标志
    private var _tempOutDate:Int = 0
    private var _humiOutDate:Int = 0

    //温湿度数据下降超过了设定值标志
    private var _tempDropAlarmFlag:Int = 0
    private var _humiDropAlarmFlag:Int = 0
    //    声音和震动报警标志
    private var voiceAlarmFlag:Int = 0
    private var vibrationAlarmFlag:Int = 0

    //    声音和震动报警使能方式
    private var voiceAlarmEnable = 0
    private var vibrationAlarmEnable = 0
    //    参数
    private var _tempDrop:Double = 0.0
    private var _humiDrop:Double = 0.0
    private var _tempmax:Double = 0.0
    private var _tempmin:Double = 0.0
    private var _humimax:Double = 0.0
    private var _humimin:Double = 0.0

    //    温湿度下降报警是否使能
    private var _humitempDrop:Int = 0
    //    超阈值报警是否使能
    private var _thresholdBeyond:Int = 0
    //    延迟
    private var _delayAlarm:Double = 0.0
    private var temp_start_once_flag = true
    private var humi_start_once_flag = true
    private var publish_start_data:String = "0"
    private var time_cali_cnt:Int = 3

    private lateinit var hhandler: Handler
    private lateinit var reConnectTask: Runnable
    private var AlarmEmergent = false
    private lateinit var wakeLock: PowerManager.WakeLock
    private var connectcnt:Int = 0
    private lateinit var bluetoothAdapter: BluetoothAdapter
    var bleTempData: List<Double>? = null
    var bleHumiData: List<Double>? = null
    var bletime:Long? = null
    var bletimebkp:Long? = null
    private val READ_INTERVAL_MS = 5 * 60 * 1000L
    private val wakeReadNow = Channel<Unit>(Channel.CONFLATED) // 触发“立刻读”的信号（合并最新）
    private var bleZeroCount = 0  // 连续0值的次数

    @Volatile private var writeInFlight = false


//    private val bluetoothAdapter: BluetoothAdapter? = BluetoothAdapter.getDefaultAdapter()
    private val deviceMacAddress = "84:C2:E4:03:02:02" // 替换为目标设备的 MAC 地址
    private val handler = Handler(Looper.getMainLooper())
//    private val executor = Executors.newSingleThreadScheduledExecutor()
    private val bluetoothScope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    private val pendingWriteQueue: java.util.concurrent.ConcurrentLinkedQueue<ByteArray> =
        java.util.concurrent.ConcurrentLinkedQueue()
    override fun onCreate() {
        super.onCreate()
        homeViewModel = HomeViewModel.getInstance(application)
        notificationViewModel = NotificationsViewModel.getInstance(application)

        createNotificationChannel()
        prefs = getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
        val macAddr  = prefs.getString("mac_addr", "000000000000")

        if (macAddr != null) {
            // 在这里使用 macAddr 数据
            processMacAddr(macAddr)
            device_mac = macAddr
        }

        val applicationContext = getApplicationContext()
        mqttClient = MqttAndroidClient(applicationContext, serverUri, clientId)
        hhandler = Handler(Looper.getMainLooper())

        val tempdropdata = prefs.getString("tempdrop", "2")
        if ((tempdropdata != "")&&(tempdropdata != null)) {
            _tempDrop = tempdropdata.toDouble()
        }
        val humdropdata = prefs.getString("humdrop", "1")
        if ((humdropdata != "")&&(humdropdata != null)) {
            _humiDrop = humdropdata.toDouble()
        }
        val tempmaxdata = prefs.getString("tempmax", "35")
        if ((tempmaxdata != "")&&(tempmaxdata != null)) {
            _tempmax = tempmaxdata.toDouble()
//            temp_before5 = _tempmax - 1
        }
        val tempmindata = prefs.getString("tempmin", "29")
        if ((tempmindata != "")&&(tempmindata != null)) {
            _tempmin = tempmindata.toDouble()
        }
        val hummaxdata = prefs.getString("hummax", "75")
        if ((hummaxdata != "")&&(hummaxdata != null)) {
            _humimax = hummaxdata.toDouble()
//            humi_before5 = _humimax - 1
        }
        val hummindata = prefs.getString("hummin", "40")
        if ((hummindata != "")&&(hummindata != null)) {
            _humimin = hummindata.toDouble()
        }
        val delayalarmdata = prefs.getString("delayalarm", "8")
        if ((delayalarmdata != "")&&(delayalarmdata != null)) {
            _delayAlarm = delayalarmdata.toDouble()
            Log.i("MqttService", "delayalarm=${_delayAlarm}")
        }


        val dengbeialarmflag = prefs.getString("dengbeialarm", "1")
        if ((dengbeialarmflag != "")&&(dengbeialarmflag != null)) {
            if(dengbeialarmflag == '1'.toString()){
                _humitempDrop = 1
            }else {
                _humitempDrop = 0
            }
        }
        val yuzialarmflag = prefs.getString("yuzhialarm", "1")
        if ((yuzialarmflag != "")&&(yuzialarmflag != null)) {
            if(yuzialarmflag == '1'.toString()){
                _thresholdBeyond = 1
            }else {
                _thresholdBeyond = 0
            }
        }
        val zhendongalarmflag = prefs.getString("zhendongalarm", "1")
        if ((zhendongalarmflag != "")&&(zhendongalarmflag != null)) {
            if(zhendongalarmflag == '1'.toString()){
                vibrationAlarmEnable = 1
            }else {
                vibrationAlarmEnable = 0
            }
        }
        val xianglingalarmflag = prefs.getString("xianglingalarm", "1")
        if ((xianglingalarmflag != "")&&(xianglingalarmflag != null)) {
            if(xianglingalarmflag == '1'.toString()){
                voiceAlarmEnable = 1
            }else {
                voiceAlarmEnable = 0
            }
        }

        val powerManager = getSystemService(Context.POWER_SERVICE) as PowerManager
        wakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "MqttService::WakeLock")
        if (!wakeLock.isHeld) {
            wakeLock.acquire()
        }

        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothAdapter = bluetoothManager.adapter
        if (macAddr != null) {
            if(!macAddr.startsWith("1010")) {
                startBluetoothTask()
            }
        }
        //startBluetoothTask()


    }

    private fun startBluetoothTask() {
        bluetoothScope.launch {
            while (isActive) {
//                if(connectcnt < 40) {
//                    connectToBluetoothDevice() // 调用挂起函数
//                }
//                connectToBluetoothDevice()
//                delay(5 * 60 * 1000) // 每5分钟延时

                select<Unit> {
                    onTimeout(READ_INTERVAL_MS) { /* 时间到了 */ }
                    wakeReadNow.onReceiveCatching { /* 收到唤醒 */ }
                }
                // 执行至少一次读取；如果在执行期间又来了触发，读完后立刻再来一轮（不等待）
                do {
                    connectToBluetoothDevice()
                    // 尝试“吃掉”在执行期间累积的触发；只要还有就继续读，不走 delay
                    val more = wakeReadNow.tryReceive().isSuccess
                } while (more)
            }
        }
    }


    private fun hex8ToBytes4(id8: String): ByteArray {
        require(Regex("^[0-9A-Fa-f]{8}$").matches(id8)) { "id must be 8-hex chars" }
        val out = ByteArray(4)
        for (i in 0 until 4) {
            out[i] = id8.substring(i*2, i*2+2).toInt(16).toByte()
        }
        return out
    }

    private fun buildWritePayload(ids: List<String>): ByteArray {
        val cleaned = ids.map { it.trim() }.filter { it.isNotEmpty() }
        require(cleaned.isNotEmpty()) { "no id" }
        val body = cleaned.flatMap { hex8ToBytes4(it).asIterable() }.toByteArray()
        return byteArrayOf(0xA1.toByte(), cleaned.size.toByte()) + body
    }

    fun buildAllSavedIdsA1SinglePacket(context: Context, maxCount: Int = 50): ByteArray? {
        val ids = MacIdBook.all(context).map { (id8, _name) -> id8 }.distinct().take(maxCount)
        if (ids.isEmpty()) return null
        require(ids.all { it.length == 8 && it.all { c -> c in "0123456789abcdefABCDEF" } })
        return buildA1Payload4B(ids)
    }

    private fun buildA1Payload4B(ids8Hex: List<String>): ByteArray {
        require(ids8Hex.isNotEmpty()) { "ids is empty" }
        require(ids8Hex.size < 256) { "count must fit in 1 byte" }

        val out = ByteArray(1 + 1 + ids8Hex.size * 4)
        var p = 0
        out[p++] = 0xA1.toByte()
        out[p++] = ids8Hex.size.toByte()

        ids8Hex.forEach { hex8 ->
            val s = hex8.uppercase()
            out[p++] = ((hexNib(s[0]) shl 4) or hexNib(s[1])).toByte()
            out[p++] = ((hexNib(s[2]) shl 4) or hexNib(s[3])).toByte()
            out[p++] = ((hexNib(s[4]) shl 4) or hexNib(s[5])).toByte()
            out[p++] = ((hexNib(s[6]) shl 4) or hexNib(s[7])).toByte()
        }
        return out
    }

    private fun hexNib(c: Char): Int = when (c) {
        in '0'..'9' -> c.code - '0'.code
        in 'A'..'F' -> c.code - 'A'.code + 10
        in 'a'..'f' -> c.code - 'a'.code + 10
        else -> error("non-hex char: $c")
    }

    private fun chunkForMtu(payload: ByteArray, mtu: Int): List<ByteArray> {
        val maxChunk = (mtu - 3).coerceAtLeast(20) // 退化保护
        val chunks = mutableListOf<ByteArray>()
        var i = 0
        while (i < payload.size) {
            val end = (i + maxChunk).coerceAtMost(payload.size)
            chunks += payload.copyOfRange(i, end)
            i = end
        }
        return chunks
    }

    private fun ByteArray.toHexSep(colon: Boolean = true): String =
        this.joinToString(if (colon) ":" else "") { "%02X".format(it) }


    private fun u16be(b: ByteArray, i: Int): Int =
        ((b[i].toInt() and 0xFF) shl 8) or (b[i + 1].toInt() and 0xFF)

    private fun s16be(b: ByteArray, i: Int): Int {
        val v = u16be(b, i)
        return if ((v and 0x8000) != 0) v - 0x10000 else v
    }


    fun parsePlainFrame(plain: ByteArray): ParsedFrame {
        require(plain.size >= 16) { "plain too short" }
        val gwid6 = plain.copyOfRange(0, 6)
        val devCount = plain[6].toInt() and 0xFF
        val expected = 16 * (1 + devCount)
        require(plain.size >= expected) { "plain length ${plain.size} < expected $expected" }

        val readings = ArrayList<DeviceReading>(devCount)
        var off = 16
        repeat(devCount) {
            val block = plain.copyOfRange(off, off + 16)
            val p8 = block.copyOfRange(0, 8)

            val devIdBytes = p8.copyOfRange(0, 4)             // 大端原样
            val devIdHex   = devIdBytes.toHexSep()
            val humi       = u16be(p8, 4) / 10.0
            val temp       = s16be(p8, 6) / 10.0

            readings += DeviceReading(devIdBytes, devIdHex, humi, temp)
            off += 16
        }
        return ParsedFrame(gwid6.toHexSep(), devCount, readings)
    }

    fun decryptAndParseEcbFrame(value: ByteArray, key16: ByteArray = APP_AES_KEY16): ParsedFrame {
        // 如果你的特征值比实际帧更长（以前尾部是 0），加密后这些 0 也会被加密成非 0。
        // 推荐 MCU 端只发送“恰好等于帧长度”的字节数；若无法控制，可在解密后按 devCount 截断。
        //Log.i("MqttService", "cipher hex (first 256B)=${value.take(256).toByteArray().toHex()}")
        val plain = aesEcbDecrypt(value, key16)

        //Log.i("MqttService", "plain hex (first 256B) = ${plain.take(256).toByteArray().toHex()}")
        // 若解密后仍多出块（例如协议栈固定长度），会被 parsePlainFrame 里的 expected 截断检查拦到
        return parsePlainFrame(plain)
    }

    private fun ByteArray.toHex(sep: String = " ") =
        joinToString(sep) { "%02X".format(it) }

    // 解密（ECB/NoPadding）
    fun aesEcbDecrypt(ciphertext: ByteArray, key16: ByteArray): ByteArray {
        Log.i("MqttService","cipher length = ${ciphertext.size}")
        require(ciphertext.size % 16 == 0) { "cipher length must be 16-byte aligned" }
        val cipher = Cipher.getInstance("AES/ECB/NoPadding")
        cipher.init(Cipher.DECRYPT_MODE, SecretKeySpec(key16, "AES"))
        return cipher.doFinal(ciphertext)
    }



    fun parseUnencryptedFrame(value: ByteArray): ParsedFrameRaw {
        require(value.size >= 16) { "frame too short" }

        // 头块：gwid6(0..5), devCount(6)
        val gwid6 = value.copyOfRange(0, 6)
        val devCount = value[6].toInt() and 0xFF

        val expected = 16 * (1 + devCount)
        require(value.size >= expected) { "frame length ${value.size} < expected $expected" }

        // 有些栈会把特征固定长度读满，后面是 0；只取前 expected 个字节
        val frame = value.copyOf(expected)

        val gatewayIdHex = gwid6.joinToString(":") { "%02X".format(it) }

        val list = ArrayList<ByteArray>(devCount)
        var off = 16
        repeat(devCount) {
            val block16 = frame.copyOfRange(off, off + 16)
            list += block16.copyOfRange(0, 8) // 前8B为有效载荷
            off += 16
        }

        return ParsedFrameRaw(gatewayIdHex, devCount, list)
    }

    fun interpretPayloadV3(payload8: ByteArray, tempSigned: Boolean = true): DeviceReadingV3 {
        require(payload8.size == 8)

        val devIdBytes = payload8.copyOfRange(0, 4)           // 大端原样保留
        val devIdHex = devIdBytes.toHexSep(true)

        val humRaw = u16be(payload8, 4)                       // 湿度 ×0.1 %RH
        val humidity = humRaw / 10.0

        val tRaw = if (tempSigned) s16be(payload8, 6) else u16be(payload8, 6)
        val temperature = tRaw / 10.0                         // 温度 ×0.1 °C

        return DeviceReadingV3(devIdBytes, devIdHex, humidity, temperature)
    }
    private fun checkBluetoothPermission(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            ActivityCompat.checkSelfPermission(this, android.Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED
        } else {
            ActivityCompat.checkSelfPermission(this, android.Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun isBluetoothEnabled(): Boolean {
        return bluetoothAdapter.isEnabled
    }

    private fun isBluetoothBusy(): Boolean {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        if (!checkBluetoothPermission()) {
            Log.i("MqttService", "Currently connected devices...")
            return true
        }
        val connectedDevices = bluetoothManager.getConnectedDevices(BluetoothProfile.GATT)

        // 如果已经有连接的设备，认为蓝牙正在被占用
        if (connectedDevices.isNotEmpty()) {
            Log.i("MqttService", "Currently connected devices: $connectedDevices")
            return true
        }

        return false
    }


    private suspend fun connectToBluetoothDevice() {
        if (!checkBluetoothPermission()) {
            Log.i("MqttService", "Bluetooth permission not granted")
            return
        }

        if (!isBluetoothEnabled()) {
            Log.i("MqttService", "Bluetooth is not enabled")
            return
        }

        if (isBluetoothBusy()) {
            Log.i("MqttService", "Bluetooth is busy with another connection")
            return
        }

        val device = bluetoothAdapter.getRemoteDevice(deviceMacAddress)
        if (device == null) {
            Log.i("MqttService", "Device not found with MAC address: $deviceMacAddress")
            return
        }

        val maxAttempts = 5
        val retryDelay = 2000L // 延迟 2 秒
        var attempt = 0
        var connected = false

        while (attempt < maxAttempts && !connected) {
            attempt++
            Log.i("MqttService", "Attempt $attempt to connect to Bluetooth device...")

            try {
                withTimeout(18000L) { // 每次连接尝试设置 10 秒超时
                    val connectionResult = connectAndReadData(device)
                    if (connectionResult) {
                        connected = true
                        Log.i("MqttService", "Successfully connected and read data on attempt $attempt")
                    }
                }
            } catch (e: TimeoutCancellationException) {
                Log.i("MqttService", "Connection attempt $attempt timed out")
            } catch (e: Exception) {
                Log.e("MqttService", "Connection attempt $attempt failed: ${e.message}", e)
            }

            if (!connected && attempt < maxAttempts) {
                Log.i("MqttService", "Retrying connection after delay...")
                delay(retryDelay) // 延迟重试
            }
        }

        if (!connected) {
            Log.i("MqttService", "Max attempts reached. Connection failed.")
        }
    }

    @SuppressLint("MissingPermission")
    private suspend fun connectAndReadData(device: BluetoothDevice): Boolean {
        var result = false
        withContext(Dispatchers.IO) {
            var gatt: BluetoothGatt? = null
            val done = CompletableDeferred<Boolean>()
            var closed = false
            var negotiatedMtu = 23



            try {
                val gattCallback = object : BluetoothGattCallback() {
                    override fun onConnectionStateChange(
                        gatt: BluetoothGatt,
                        status: Int,
                        newState: Int
                    ) {
                        if (status != BluetoothGatt.GATT_SUCCESS) {
                            Log.i("MqttService", "Connect error status=$status")
                            done.complete(false);  return
                        }

                        if (newState == BluetoothProfile.STATE_CONNECTED) {


                            Log.i("MqttService", "Connected to device, discovering services...")
                            if (checkBluetoothPermission()) {
//                                gatt.discoverServices()
                                gatt.requestMtu(240)
                            }
                        } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                            if (!done.isCompleted) done.complete(false)
                            Log.i("MqttService", "Disconnected from device")
                        }
                    }
                    override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
                        negotiatedMtu = if (status == BluetoothGatt.GATT_SUCCESS) mtu else 23
                        Log.i("MqttService", "MTU negotiated: $negotiatedMtu (status=$status)")
                        g.discoverServices()
                    }

                    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
                        if (status == BluetoothGatt.GATT_SUCCESS) {
                            val service =
                                gatt.getService(UUID.fromString("0000ffe0-0000-1000-8000-00805F9B34FB"))
                            if (service != null) {
                                val characteristic =
                                    service.getCharacteristic(UUID.fromString("0000ffe2-0000-1000-8000-00805F9B34FB"))
                                if (characteristic != null && checkBluetoothPermission()) {
                                    gatt.readCharacteristic(characteristic)
                                }
                            }
                        }
                    }

                    override fun onCharacteristicWrite(
                        gatt: BluetoothGatt,
                        characteristic: BluetoothGattCharacteristic,
                        status: Int
                    ) {
                        writeInFlight = false
                        if (status == BluetoothGatt.GATT_SUCCESS) {
                            result = true
                            if (!done.isCompleted) done.complete(true)
                            Log.i("MqttService", "Chunk written OK, left=${pendingWriteQueue.size}")
                        } else {
                            Log.e("MqttService", "Write failed status=$status, left=${pendingWriteQueue.size}")
                            // 失败策略：清空 or 重试；这里简单清空
                            //pendingWriteQueue.clear()
                            Log.e("MqttService", "Write failed status=$status")
                            if (!done.isCompleted) done.complete(false)
                        }
                        // 继续下一块/或收尾
                        //maybeWriteNext(gatt)
                        safelyDisconnectAndClose(gatt)
                    }

                    @Suppress("DEPRECATION")
                    override fun onCharacteristicRead(
                        gatt: BluetoothGatt,
                        characteristic: BluetoothGattCharacteristic,
                        status: Int
                    ) {
                        if (status == BluetoothGatt.GATT_SUCCESS) {
                            val value = characteristic.value
//                            val hexString =
//                                value.joinToString(separator = " ") { String.format("%02X", it) }
////                            val (humidityList, temperatureList) = parseHexData(hexString)
////                            processTemperatureHumidityData(temperatureList, humidityList)
//                            Log.i("MqttService", "Received data in hex: $hexString")
//
//                            val parsed = parseUnencryptedFrame(value)
//                            Log.i("MqttService", "GwID=${parsed.gatewayIdHex}, devCount=${parsed.devCount}")
//
//                            // 2) 如果你的 8B 就是 devId/温度/湿度 这个布局：
//                            parsed.payloadsRaw8.forEachIndexed { idx, p8 ->
//                                val r = interpretPayloadV3(p8, tempSigned = true) // 温度需要负值就 true
//                                Log.i("MqttService", "dev#$idx id=${r.devIdHex}, H=${"%.1f".format(r.humidityPct)}%, T=${"%.1f".format(r.temperatureC)}°C")
//                                processTemperatureHumidityData(r.temperatureC, r.humidityPct)
//                            }
                            try {
                                val parsed = decryptAndParseEcbFrame(value)
                                Log.i("MqttService", "GwID=${parsed.gatewayIdHex}, dev=${parsed.devCount}")
                                parsed.readings.forEach { r ->
                                    Log.i("MqttService", " id=${r.devIdHex}, H=${"%.1f".format(r.humidityPct)}%, T=${"%.1f".format(r.temperatureC)}°C")

                                    val temperature = r.temperatureC
                                    val humidity = r.humidityPct
                                    val isZeroValue = temperature == 0.0 && humidity == 0.0

                                    // devId 从 4 字节原始值转 8 位 HEX 大写无分隔(匹配 MacIdBook)
                                    val devId = r.devIdHex.replace(":", "")

                                    // BLE publish 用:记录最后一次读数(老行为,多设备时只发最后一个)
                                    // connectToBroker 里会读取这 3 个变量把 BLE 数据转 MQTT
                                    bletime = System.currentTimeMillis() / 1000
                                    bleTempData = listOf(temperature)
                                    bleHumiData = listOf(humidity)

                                    if (isZeroValue) {
                                        bleZeroCount++
                                        if (bleZeroCount >= 3) {
                                            processTemperatureHumidityData(devId, r.temperatureC, r.humidityPct)
                                        }
                                    } else {
                                        bleZeroCount = 0
                                        processTemperatureHumidityData(devId, r.temperatureC, r.humidityPct)
                                    }
                                }
                            } catch (e: Exception) {
                                Log.e("MqttService", "Decrypt/parse failed: ${e.message}", e)
                            }

                            val started = maybeWriteNext(gatt)
                            if (!started) {
                                // 没有要写的/写失败：在这里收尾
                                result = true
                                if (!done.isCompleted) done.complete(true)
                                safelyDisconnectAndClose(gatt)
                            }

                            //result = true

//                            Log.i("MqttService", "Humidity List: $humidityList")
//                            Log.i("MqttService", "Temperature List: $temperatureList")
                            //done.complete(true)
                        } else {
                            if (!done.isCompleted) done.complete(false)
                            //done.complete(false)
                            Log.i("MqttService", "Failed to read characteristic")
                        }
//                        if (checkBluetoothPermission()) {
//                            safelyDisconnectAndClose(gatt)
//                        }
                    }
                }

                gatt = device.connectGatt(applicationContext, false, gattCallback)
                withTimeout(16_000) { done.await() }
            } catch (e: Exception) {
                Log.e("MqttService", "Error during connection or data read: ${e.message}", e)
            } finally {
                safelyDisconnectAndClose(gatt)
            }
        }
        return result

    }
    @RequiresPermission(value = "android.permission.BLUETOOTH_CONNECT")
    private fun safelyDisconnectAndClose(gatt: BluetoothGatt?) {
        try {
            gatt?.disconnect()
            gatt?.close()
        } catch (e: Exception) {
            Log.e("MqttService", "Error while closing BluetoothGatt: ${e.message}", e)
        }
    }


    fun parseHexData(hexData: String): Pair<List<Int>, List<Int>> {
        val humidityList = mutableListOf<Int>()
        val temperatureList = mutableListOf<Int>()

        // 将 16 进制字符串分割为字节对
        val bytes = hexData.split(" ").map { it.toInt(16) }

        for (i in bytes.indices step 4) {
            if (i + 3 < bytes.size) {
                // 湿度数据
                val humidity = bytes[i] or (bytes[i + 1] shl 8)
                // 温度数据
                val temperature = bytes[i + 2] or (bytes[i + 3] shl 8)

                humidityList.add(humidity)
                temperatureList.add(temperature)
            }
        }

        return Pair(humidityList, temperatureList)
    }

    // 处理温度湿度数据,包括报警逻辑。每个设备独立调用。
    // devId 必须是 8 位 HEX 大写无分隔(与 MacIdBook 一致)。
    // timeOverride: MQTT 路径用 payload 里的时间(网关上报时间),BLE 路径不传(用系统时间)。
    fun processTemperatureHumidityData(devId: String, temperature: Double, humidity: Double, timeOverride: Long? = null) {
        try {
            val currentTime = timeOverride ?: (System.currentTimeMillis() / 1000) // Unix 秒

            val history = deviceHistory.getOrPut(devId) { DeviceHistory() }

            // 30 分钟过期检查(老逻辑保留)
            if (history.data5MinAgo != null && currentTime - history.data5MinAgo!!.timestamp > 30 * 60) {
                history.data5MinAgo = null
            }
            if (history.data10MinAgo != null && currentTime - history.data10MinAgo!!.timestamp > 30 * 60) {
                history.data10MinAgo = null
            }
            if (history.data15MinAgo != null && currentTime - history.data15MinAgo!!.timestamp > 30 * 60) {
                history.data15MinAgo = null
            }

            // 清屏:5/10/15 历史都为空(无参照点)时,清掉这个设备的 sparkline
            if (history.data5MinAgo == null && history.data10MinAgo == null && history.data15MinAgo == null) {
                homeViewModel.clearDevice(devId)
            }

            val newData = HumidityTemperatureData(currentTime, temperature, humidity)

            // 每设备独立报警参数(未设置的字段 fallback 到 DashboardFragment 全局默认值)
            val params = loadDeviceParams(devId)

            // 报警逻辑
            vibrationAlarmFlag = 0
            voiceAlarmFlag = 0
            var alarmMessage = "未设置报警"
            var alarmSeverity = Severity.NORMAL

            if ((params.humitempDrop == 0) && (params.thresholdBeyond == 0)) {
                alarmMessage = "未设置报警"
                alarmSeverity = Severity.NORMAL
            } else if ((params.humitempDrop == 1) && (params.thresholdBeyond == 0)) {
                val isSignificantChange5MinAgo = history.data5MinAgo?.let {
                    it.temperature - newData.temperature > params.tempDrop && it.humidity - newData.humidity > params.humiDrop
                } ?: false
                val isSignificantChange10MinAgo = history.data10MinAgo?.let {
                    it.temperature - newData.temperature > params.tempDrop && it.humidity - newData.humidity > params.humiDrop
                } ?: false
                val isSignificantChange15MinAgo = history.data15MinAgo?.let {
                    it.temperature - newData.temperature > params.tempDrop && it.humidity - newData.humidity > params.humiDrop
                } ?: false
                if (isSignificantChange5MinAgo || isSignificantChange10MinAgo || isSignificantChange15MinAgo) {
                    alarmMessage = "温度湿度下降报警"
                    alarmSeverity = Severity.WARNING
                    if (params.voiceAlarmEnable == 1) voiceAlarmFlag = 1
                    if (params.vibrationAlarmEnable == 1) vibrationAlarmFlag = 1
                }
            } else if ((params.humitempDrop == 0) && (params.thresholdBeyond == 1)) {
                if (newData.temperature > params.tempmax || newData.temperature < params.tempmin
                    || newData.humidity > params.humimax || newData.humidity < params.humimin) {
                    alarmMessage = "温湿度超限报警"
                    alarmSeverity = Severity.WARNING
                    if (params.voiceAlarmEnable == 1) voiceAlarmFlag = 1
                    if (params.vibrationAlarmEnable == 1) vibrationAlarmFlag = 1
                }
            } else {
                // 两种报警都启用
                val isSignificantChange5MinAgo = history.data5MinAgo?.let {
                    it.temperature - newData.temperature > params.tempDrop && it.humidity - newData.humidity > params.humiDrop
                } ?: false
                val isSignificantChange10MinAgo = history.data10MinAgo?.let {
                    it.temperature - newData.temperature > params.tempDrop && it.humidity - newData.humidity > params.humiDrop
                } ?: false
                val isSignificantChange15MinAgo = history.data15MinAgo?.let {
                    it.temperature - newData.temperature > params.tempDrop && it.humidity - newData.humidity > params.humiDrop
                } ?: false
                if (isSignificantChange5MinAgo || isSignificantChange10MinAgo || isSignificantChange15MinAgo) {
                    alarmMessage = "温度湿度下降报警"
                    alarmSeverity = Severity.WARNING
                    if (params.voiceAlarmEnable == 1) voiceAlarmFlag = 1
                    if (params.vibrationAlarmEnable == 1) vibrationAlarmFlag = 1
                }
                if (newData.temperature > params.tempmax || newData.temperature < params.tempmin
                    || newData.humidity > params.humimax || newData.humidity < params.humimin) {
                    alarmMessage = "温湿度超限报警"
                    alarmSeverity = Severity.WARNING
                    if (params.voiceAlarmEnable == 1) voiceAlarmFlag = 1
                    if (params.vibrationAlarmEnable == 1) vibrationAlarmFlag = 1
                }
            }

            // 紧急报警(温度 > 65°C 火灾预警)
            AlarmEmergent = false
            if (newData.temperature > 65f) {
                AlarmEmergent = true
                alarmMessage = "紧急报警，温度超过65度，谨防火灾"
                alarmSeverity = Severity.ALARM
                VibrationPlayer.vibratePhone(this@MqtttService, 300 * 1000, true)
                RingtonePlayer.startAlarm(this@MqtttService)
                // 紧急报警每次都发通知(不受节流限制)
                val emergentDeviceName = MacIdBook.all(applicationContext).find { it.first == devId }?.second ?: devId
                sendAlarmNotification(devId, emergentDeviceName, alarmMessage, alarmSeverity)
            }

            // 推送给 UI(更新 _devices Map,卡片墙 observe)
            homeViewModel.updateDeviceAlarm(devId, alarmMessage, alarmSeverity)

            // 报警事件落库(只记 WARNING/ALARM,NORMAL 不记)
            when (alarmMessage) {
                "温度湿度下降报警" -> storeAlarmEvent(
                    this@MqtttService, devId,
                    TemperatureDatabaseHelper.ALARM_TYPE_DROP, alarmMessage, currentTime
                )
                "温湿度超限报警" -> storeAlarmEvent(
                    this@MqtttService, devId,
                    TemperatureDatabaseHelper.ALARM_TYPE_THRESHOLD, alarmMessage, currentTime
                )
                "紧急报警，温度超过65度，谨防火灾" -> storeAlarmEvent(
                    this@MqtttService, devId,
                    TemperatureDatabaseHelper.ALARM_TYPE_EMERGENCY, alarmMessage, currentTime
                )
            }

            // 存储逻辑
            val averageList0 = mutableListOf(temperature)
            val averageList1 = mutableListOf(humidity)
            val humistartflag = homeViewModel.startData.value
            if (humistartflag != null) {
                storeTemperatureData(
                    this@MqtttService,
                    averageList0,
                    averageList1,
                    currentTime,
                    devId,
                    humistartflag
                )
            }

            // 显示逻辑:推送给新的 _devices Map
            val deviceName = MacIdBook.all(applicationContext).find { it.first == devId }?.second ?: ""
            homeViewModel.updateDeviceReading(devId, deviceName, temperature, humidity, currentTime)

            // 数据更新(滑动历史窗口)
            if (history.data10MinAgo != null) {
                history.data15MinAgo = history.data10MinAgo
            }
            if (history.data5MinAgo != null) {
                history.data10MinAgo = history.data5MinAgo
            }
            history.data5MinAgo = newData
            _bkp_all_time = currentTime

            // 报警处理:震动 + 铃声 + 系统通知,按设备节流(900 秒)
            if (homeViewModel.startData.value == 1) {
                if (vibrationAlarmFlag == 1 || voiceAlarmFlag == 1) {
                    val now = System.currentTimeMillis()
                    if (now - history.lastAlarmTime > (900 * 1000)) {
                        history.lastAlarmTime = now
                        if (vibrationAlarmFlag == 1) {
                            VibrationPlayer.vibratePhone(this@MqtttService, 300 * 1000, true)
                        }
                        if (voiceAlarmFlag == 1) {
                            RingtonePlayer.startAlarm(this@MqtttService)
                        }
                        // 节流期内只发一次通知:告诉用户是哪个设备在报警
                        val throttledDeviceName = MacIdBook.all(applicationContext).find { it.first == devId }?.second ?: devId
                        sendAlarmNotification(devId, throttledDeviceName, alarmMessage, alarmSeverity)
                    }
                }
            }

        } catch (e: Exception) {
            Log.e("DataProcessor", "Error processing data", e)
        }
    }

    private fun processMacAddr(macAddr: String) {
        // 处理 macAddr 数据
        // 更新 topic
        rtopic0 = "/$macAddr/a/r0" // 修改 topic 的值
        rtopic1 = "/$macAddr/a/r1"
        stopic0 = "/$macAddr/a/s0"
    }


    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val notification = createNotification()
        startForeground(NOTIFICATION_ID, notification)

        serviceScope.launch {
            val action = intent?.action
//            connectToBroker()
            if (action == "PUBLISH_DATA") {
                val tempdropdata = intent.getStringExtra("tempdrop")
                val humdropdata = intent.getStringExtra("humdrop")
                val tempmaxdata = intent.getStringExtra("tempmax")
                val tempmindata = intent.getStringExtra("tempmin")
                val hummaxdata = intent.getStringExtra("hummax")
                val hummindata = intent.getStringExtra("hummin")
                val guardtimedata = intent.getStringExtra("guardtime")
                val delayalarmdata = intent.getStringExtra("delayalarm")
                val dengbeialarmdata = intent.getStringExtra("dengbeialarm")
                val yuzhialarmdata = intent.getStringExtra("yuzhialarm")


                if (tempdropdata != null && humdropdata != null && tempmaxdata != null
                    && tempmindata != null && hummaxdata != null && hummindata != null && guardtimedata != null
                    && delayalarmdata != null && dengbeialarmdata != null && yuzhialarmdata != null
                ) {
                    val payload = "$tempdropdata,$humdropdata,$tempmaxdata," +
                            "$tempmindata,$hummaxdata,$hummindata,$guardtimedata,$delayalarmdata,$dengbeialarmdata,$yuzhialarmdata"
                    publishMessage(payload, stopic0)
                }
            }

            if(action == "START_STOP_GUARD"){
                val ga = intent.getStringExtra("guardaction")
                publish_start_data = "$ga"
                time_cali_cnt = 3
            }

            if (action == "BLE_ENQUEUE_WRITE_IDS") {
                val list = intent.getStringArrayListExtra("ids") ?: arrayListOf()
                val single = intent.getStringExtra("id")
                val ids = (list + listOfNotNull(single)).distinct()

                if (ids.isNotEmpty()) {
                    try {
                        val payload = buildWritePayload(ids)
                        // 先不立刻写；只入队，等待本轮 read 完成后触发
                        val chunks = chunkForMtu(payload, 240)
                        chunks.forEach { pendingWriteQueue.offer(it) }
                        wakeReadNow.trySend(Unit)
                        Log.i(
                            "MqttService",
                            "Enqueued ${chunks.size} chunk(s) for ${ids.size} id(s)"
                        )
                    } catch (e: Exception) {
                        Log.e("MqttService", "enqueue ids failed: ${e.message}", e)
                    }
                }
            }

        }
        scheduleReconnect()

        return START_STICKY
    }


    private fun handlePublishDataAction() {


    }
    @RequiresPermission(android.Manifest.permission.BLUETOOTH_CONNECT)
    private fun maybeWriteNext(gatt: BluetoothGatt?): Boolean {
        if (gatt == null) return false
        val service = gatt.getService(GW_SERVICE_UUID) ?: return false
        val writeChar = service.getCharacteristic(GW_WRITE_UUID) ?: return false

        val payload = buildAllSavedIdsA1SinglePacket(applicationContext, maxCount = 50) ?: run {
            Log.i("MqttService", "No ids to write")
            return false // 没有要写的
        }

        // 选择写入类型
        val props = writeChar.properties
        val supportsWrite = (props and BluetoothGattCharacteristic.PROPERTY_WRITE) != 0
        val supportsWnr   = (props and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE) != 0
        if (!supportsWrite && !supportsWnr) {
            Log.e("MqttService", "Write not permitted")
            return false
        }
        writeChar.writeType = if (supportsWrite)
            BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        else
            BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE

        // 发送（API 33 区分新旧）
        writeInFlight = true
        val ok = if (Build.VERSION.SDK_INT >= 33) {
            val code = gatt.writeCharacteristic(writeChar, payload, writeChar.writeType)
            Log.i("MqttService", "writeCharacteristic(API33+) code=$code, bytes=${payload.size}")
            code == android.bluetooth.BluetoothStatusCodes.SUCCESS
        } else {
            @Suppress("DEPRECATION")
            run {
                writeChar.value = payload
                val okLegacy = gatt.writeCharacteristic(writeChar)
                Log.i("MqttService", "writeCharacteristic(legacy) ok=$okLegacy, bytes=${payload.size}")
                okLegacy
            }
        }
        if (!ok) {
            writeInFlight = false
            return false
        }

        // 无响应写：不会有回调，我们自己延时收尾
        if (writeChar.writeType == BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE) {
            serviceScope.launch {
                delay(20) // 给底层一点时间
                writeInFlight = false
                safelyDisconnectAndClose(gatt)
                // 如果你想把 connectAndReadData 的 done 也放到这里：done.complete(true)
            }
        }
        return true
    }


    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                CHANNEL_ID,
                "MQTT Service Channel",
                NotificationManager.IMPORTANCE_DEFAULT
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager.createNotificationChannel(serviceChannel)

            // 早起报告 channel(Phase 3 加)
            val reportChannel = NotificationChannel(
                "daily_report_channel",
                "早起报告",
                NotificationManager.IMPORTANCE_DEFAULT
            ).apply {
                description = "每日早上的数据汇总报告"
            }
            manager.createNotificationChannel(reportChannel)

            // 报警 channel(多设备支持:告诉用户是哪个设备在报警)
            val alarmChannel = NotificationChannel(
                "alarm_channel",
                "设备报警",
                NotificationManager.IMPORTANCE_HIGH
            ).apply {
                description = "温湿度超限/下降/紧急报警"
                enableVibration(true)
                vibrationPattern = longArrayOf(0, 500, 200, 500)
                lockscreenVisibility = android.app.Notification.VISIBILITY_PUBLIC
            }
            manager.createNotificationChannel(alarmChannel)
        }
    }

    /**
     * 发送一条系统通知,告诉用户是哪个设备在报警。
     * 用 devId 的 hashcode 做 notificationId,保证不同设备不互相覆盖,但同设备多次报警会更新同一条。
     */
    private fun sendAlarmNotification(
        devId: String,
        deviceName: String,
        alarmMessage: String,
        severity: com.jinyuni.dengbei_care.Severity
    ) {
        val ctx = this@MqtttService
        val nm = NotificationManagerCompat.from(ctx)
        if (!nm.areNotificationsEnabled()) return

        val title = if (deviceName.isNotBlank()) {
            "[$deviceName] 报警"
        } else {
            "[$devId] 报警"
        }

        // 点击通知打开 MainActivity(卡片墙),用户能看到红卡片
        val launchIntent = Intent(ctx, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP
        }
        val pendingIntent = PendingIntent.getActivity(
            ctx,
            devId.hashCode(),
            launchIntent,
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT
        )

        val colorRes = when (severity) {
            com.jinyuni.dengbei_care.Severity.ALARM -> com.jinyuni.dengbei_care.R.color.brand_error
            com.jinyuni.dengbei_care.Severity.WARNING -> com.jinyuni.dengbei_care.R.color.brand_warning
            else -> com.jinyuni.dengbei_care.R.color.brand_primary
        }

        val notification = NotificationCompat.Builder(ctx, "alarm_channel")
            .setSmallIcon(com.jinyuni.dengbei_care.R.drawable.ic_launcher_foreground)
            .setContentTitle(title)
            .setContentText(alarmMessage)
            .setStyle(NotificationCompat.BigTextStyle().bigText("$alarmMessage\n设备: $deviceName"))
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setColor(ctx.getColor(colorRes))
            .setColorized(true)
            .setContentIntent(pendingIntent)
            .setAutoCancel(true)
            .setCategory(NotificationCompat.CATEGORY_ALARM)
            .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
            .build()

        try {
            nm.notify(devId.hashCode(), notification)
        } catch (e: Exception) {
            Log.w("AlarmNotify", "Failed to post alarm notification", e)
        }
    }

    private fun createNotification(): Notification {
        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this,
            0,
            notificationIntent,
            PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("MQTT Service")
            .setContentText("Running...")
            .setContentIntent(pendingIntent)
            .build()
    }
    private suspend fun disconnectFromBroker() {
        withContext(Dispatchers.IO) {
            try {

                mqttClient.close()
                // 您可能还需要在这里执行其他与资源释放相关的操作
                Log.d("MqttService", "Disconnected from broker and resources released")
            } catch (e: MqttException) {
                Log.e("MqttService", "Error disconnecting or closing", e)
            }
        }
    }


    private suspend fun connectToBroker() {
        // 将原 MainActivity 中的 connectToBroker 逻辑移到这里
        withContext(Dispatchers.IO) {
            val options = setupSSL()
//            options.keepAliveInterval = 5
//            options.isAutomaticReconnect = true
            options.connectionTimeout = 30

            try {
                val token: IMqttToken = mqttClient.connect(options)

                token.actionCallback = object : IMqttActionListener {
                    override fun onSuccess(asyncActionToken: IMqttToken) {
                        // 连接成功
                        Log.d("MqttService", "Connected to broker")

                        if((device_mac != "") &&(!device_mac.startsWith("1010"))){
                            if(bletime != bletimebkp){
                                bletimebkp = bletime
                                val size0 = bleHumiData?.size
                                val size1 = bleTempData?.size
                                if((bleHumiData != null && size0 != 0) && (
                                            bleTempData != null && size1 != 0
                                        )){
                                    val resulth: String? = bleHumiData?.map { "%.1f".format(it) }?.joinToString(", ")
                                    val resultt: String? = bleTempData?.map { "%.1f".format(it) }?.joinToString(", ")
                                    val payload = bletime.toString() + ',' + "$resulth,$resultt"
                                    publishMessage(payload, rtopic0)
                                }
                            }
                        }else {

                            subscribeToTopic(rtopic0)
                        }
                        connectcnt = 0
//                        subscribeToTopic(rtopic1)
                    }

                    override fun onFailure(asyncActionToken: IMqttToken, exception: Throwable) {
                        // 连接失败
                        Log.e("MqttService", "Failed to connect to broker", exception)
                        notificationViewModel._mqtt_broker_state.value = "网关：断网"
                        connectcnt++
//                        scheduleReconnect()
                    }
                }
            } catch (e: MqttException) {
                Log.e("MqttService", "MQTT connection exception", e)
                notificationViewModel._mqtt_broker_state.value = "网关：断网"
            }
        }
    }

    private fun setupSSL(): MqttConnectOptions {
        val options = MqttConnectOptions()

        try {
            val assetManager = assets
            assetManager.open("jd-ca.crt").use {
                val certificateFactory = CertificateFactory.getInstance("X.509")
                val certificate = certificateFactory.generateCertificate(it)
                val trustStore = KeyStore.getInstance(KeyStore.getDefaultType())
                trustStore.load(null, null)
                trustStore.setCertificateEntry("ca", certificate)

                val trustManagerFactory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm())
                trustManagerFactory.init(trustStore)

                val sslContext = SSLContext.getInstance("TLSv1.2")
                sslContext.init(null, trustManagerFactory.trustManagers, null)
                options.socketFactory = sslContext.socketFactory
                val prefs = getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
                val savedInput = prefs.getString("mac_addr", "000000000000")
                Log.i("MqttService", "Saved MAC address: $savedInput")
                if ((savedInput != null)&&(savedInput != "")) {
                    options.userName = savedInput
                    options.password = (savedInput + "&^!A:z?").toCharArray()
                }else{
                    options.userName = ""
                    options.password = "".toCharArray()
                }
//                options.isAutomaticReconnect = true
//                options.setAutomaticReconnect(true);
                options.keepAliveInterval = 20
            }


        } catch (e: Exception) {
            Log.e("MqttService", "SSL setup failed", e)
        }

        return options
    }

    private fun subscribeToTopic(topic: String) {
        try {
            mqttClient.subscribe(topic, 1, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken) {
                    Log.d("MqttService", "Subscribed to $topic")
                    notificationViewModel._mqtt_broker_state.value = "网关：已联网"
                }

                override fun onFailure(asyncActionToken: IMqttToken, exception: Throwable) {
                    Log.e("MqttService", "Failed to subscribe to $topic", exception)
                    notificationViewModel._mqtt_broker_state.value = "网关：断网"
                }
            })

            mqttClient.setCallback(object : MqttCallback {
                override fun connectionLost(cause: Throwable) {
                    Log.d("MqttService", "Connection lost")
                    mqttClient.unregisterResources()
//                    scheduleReconnect()
                }

                override fun messageArrived(topic: String, message: MqttMessage) {
                    Log.d("MqttService", "Message received: ${String(message.payload)},${topic}")

//                    homeViewModel._mqttData.value = String(message.payload)
//                    homeViewModel._mqttTopic.value = topic

//                    homeViewModel._mqttData.value = String(message.payload)+":"+topic
                    val temppayload: String = String(message.payload)
                    val temptopic: String = topic

                    try {
                        val parts = temppayload.split(":")

                        if(parts.size >= 3) {
                            val tempCurrentTime = parts[0].toLongOrNull()
                            if (tempCurrentTime != null) {
                                currentTime = tempCurrentTime
                            }
                            if (parts[1].isNotBlank()) {
                                readBackTemperature = parts[1]
                            }

                            if (parts[2].isNotBlank()) {
                                readBackHuminity = parts[2]
                            }
                        }
                    }catch (e: Exception) {
                        Log.e("MqttService", "Error processing payload: $temppayload", e)
                    }

                    readbacktopic = temptopic
                    val device_nu = isSecondSlashFollowedByA(readbacktopic)
                    if (device_nu == 'a') {
                        if (_bkp_temp_time != currentTime) {
                            _bkp_temp_time = currentTime

                            // 解析温度列表(payload 格式 time:temp1,temp2,...:humi1,humi2,...)
                            val tempDoubles0: List<Double>? = try {
                                readBackTemperature.split(",").map { it.toDouble() }
                            } catch (e: Exception) {
                                Log.e("MqttService", "Error converting temperature data", e)
                                null
                            }
                            val doubles0 = tempDoubles0 ?: emptyList()

                            val tempDoubles1: List<Double>? = try {
                                readBackHuminity.split(",").map { it.toDouble() }
                            } catch (e: Exception) {
                                Log.e("MqttService", "Error converting humidity data", e)
                                null
                            }
                            val doubles1 = tempDoubles1 ?: emptyList()

                            if (tempDoubles0.isNullOrEmpty() || tempDoubles1.isNullOrEmpty()) {
                                Log.e("MqttService", "receive Error data")
                                return
                            }

                            // 多设备路由:按 MacIdBook.all() 顺序位置匹配
                            // payload 格式不带 devId,假设网关按 saved IDs 顺序回传
                            val savedIds = MacIdBook.all(applicationContext)
                            if (savedIds.isEmpty()) {
                                Log.w("MqttService", "MQTT payload received but no saved device IDs - dropping")
                                return
                            }

                            val count = minOf(savedIds.size, doubles0.size, doubles1.size)
                            if (savedIds.size != doubles0.size || doubles0.size != doubles1.size) {
                                Log.w("MqttService", "MQTT device count mismatch: saved=${savedIds.size}, temps=${doubles0.size}, humis=${doubles1.size} - using first $count")
                            }

                            for (i in 0 until count) {
                                val devId = savedIds[i].first
                                processTemperatureHumidityData(devId, doubles0[i], doubles1[i], currentTime)
                            }
                        }
                    }


                    time_cali_cnt++
                    if (time_cali_cnt >= 1) {
                        val currtime = System.currentTimeMillis()
                        val payload =
                            publish_start_data + ",$currtime,3.1,3.1,3.1,3.1,3.1,3.1,1,1"
                        publishMessage(payload, stopic0)
//                        if_publish_data = false
                        time_cali_cnt = 0
                        Log.i("MqttService","publish finished")
                    }

                }

                override fun deliveryComplete(token: IMqttDeliveryToken) {
                    Log.d("MqttService", "Message delivered")
                }
            })
        } catch (e: MqttException) {
            Log.e("MqttService", "Subscription failed", e)
        }
    }

    private fun isSecondSlashFollowedByA(input: String): Char? {
        // 找到第一个 '/' 的位置
        val firstSlashIndex = input.indexOf('/')
        if (firstSlashIndex == -1) return 'x' // 如果没有找到 '/'，返回 false

        // 找到第二个 '/' 的位置
        val secondSlashIndex = input.indexOf('/', startIndex = firstSlashIndex + 1)
        if (secondSlashIndex == -1) return 'x' // 如果没有找到第二个 '/'，返回 false

        // 检查第二个 '/' 后面的字符是否是 'a'
        return input.getOrNull(secondSlashIndex + 1)
    }

    private fun isThirdSlashFollowedByA(input: String): Char? {
        // 找到第一个 '/' 的位置
        val firstSlashIndex = input.indexOf('/')
        if (firstSlashIndex == -1) return 'x' // 如果没有找到 '/'，返回 false

        // 找到第二个 '/' 的位置
        val secondSlashIndex = input.indexOf('/', startIndex = firstSlashIndex + 1)
        if (secondSlashIndex == -1) return 'x' // 如果没有找到第二个 '/'，返回 false

        val thirdSlashIndex = input.indexOf('/', startIndex = secondSlashIndex + 1)
        if (thirdSlashIndex == -1) return 'x' // 如果没有找到第二个 '/'，返回 false

        // 检查第二个 '/' 后面的字符是否是 'a'
        return input.getOrNull(thirdSlashIndex + 2)
    }

    fun publishMessage(payload: String, topic: String) {
        if (mqttClient.isConnected) {
            val message = MqttMessage()
            message.payload = payload.toByteArray()
            message.qos = 1

            try {
                mqttClient.publish(topic, message)
            } catch (e: MqttException) {
                Log.e("MqttService", "Failed to publish message", e)
            }
        }else{
            Log.e("MqttService", "not connect ,Failed to publish message")
        }
    }

    private fun scheduleReconnect() {
        serviceScope.launch {
            while(isActive) {
                delay(reconnectInterval)
                if (!mqttClient.isConnected) {

                    Log.d("MqttService", "Attempting to reconnect to broker")
                    try {
                        connectToBroker()
                    } finally {

                    }
                }
            }
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onDestroy() {
        super.onDestroy()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            stopForeground(STOP_FOREGROUND_REMOVE)
        } else {
            @Suppress("DEPRECATION")
            stopForeground(true)
        }

//        serviceScope.cancel()
//        disconnectFromBroker()
        Log.d("MqttService", "Service destroyed")
        try {
            serviceScope.cancel()
//            executor.shutdown()
            bluetoothScope.cancel() // 取消协程
            mqttClient.disconnect()
            if (wakeLock.isHeld) {
                wakeLock.release()
            }
        } catch (e: MqttException) {
            Log.e("MqttService", "Error disconnecting from MQTT broker", e)
        }
        hhandler.removeCallbacksAndMessages(null)
//        if (wakeLock.isHeld) {
//            wakeLock.release()
//        }
        DatabaseHelperInstance.getDatabaseHelper(applicationContext).close()
        // 可以在这里重新启动服务
    }
}