package com.jinyuni.dengbei_care

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import com.github.mikephil.charting.data.Entry

// 设备一天的统计摘要(给早起报告用)
data class DailyStats(
    val devId: String,
    val sampleCount: Int,
    val tempMin: Double,
    val tempMax: Double,
    val tempAvg: Double,
    val humiMin: Double,
    val humiMax: Double,
    val humiAvg: Double
)

// 报警事件(给早起报告 + 详情页报警列表用)
data class AlarmEvent(
    val id: Long,
    val time: Long,         // Unix 秒
    val devId: String,
    val type: String,        // ALARM_TYPE_*
    val message: String
)

class TemperatureDatabaseHelper(context: Context, name: String, factory: SQLiteDatabase.CursorFactory?, version: Int) :
    SQLiteOpenHelper(context, name, factory, version) {

    private val appContext = context.applicationContext

    companion object TemperatureDatabaseHelper {
        const val DATABASE_NAME = "bwtemperature_database1"
        const val DATABASE_VERSION = 2

        // 单表替代老版本的 temperatureA/B/C 三表
        const val TABLE_TEMPERATURE = "temperature"
        const val COLUMN_TIME = "time"
        const val COLUMN_DEVICE_ID = "device_id"
        const val COLUMN_TEMPERATURE = "temperature"
        const val COLUMN_HUMIDITY = "humidity"

        // 报警事件表
        const val TABLE_ALARM_EVENTS = "alarm_events"
        const val COLUMN_ALARM_ID = "id"
        const val COLUMN_ALARM_TYPE = "type"
        const val COLUMN_ALARM_MESSAGE = "message"

        // 报警类型常量
        const val ALARM_TYPE_DROP = "drop"           // 温湿度下降报警
        const val ALARM_TYPE_THRESHOLD = "threshold" // 温湿度超限报警
        const val ALARM_TYPE_EMERGENCY = "emergency" // 紧急报警(温度>65°C 火灾预警)

        private const val SQL_CREATE_TABLE_TEMPERATURE =
            "CREATE TABLE $TABLE_TEMPERATURE (" +
                    "$COLUMN_TIME INTEGER, " +
                    "$COLUMN_DEVICE_ID TEXT, " +
                    "$COLUMN_TEMPERATURE REAL, " +
                    "$COLUMN_HUMIDITY REAL, " +
                    "PRIMARY KEY($COLUMN_TIME, $COLUMN_DEVICE_ID))"

        private const val SQL_CREATE_TABLE_ALARM_EVENTS =
            "CREATE TABLE $TABLE_ALARM_EVENTS (" +
                    "$COLUMN_ALARM_ID INTEGER PRIMARY KEY AUTOINCREMENT, " +
                    "$COLUMN_TIME INTEGER, " +
                    "$COLUMN_DEVICE_ID TEXT, " +
                    "$COLUMN_ALARM_TYPE TEXT, " +
                    "$COLUMN_ALARM_MESSAGE TEXT)"

        private const val SQL_CREATE_INDEX_TEMPERATURE_DEVICE =
            "CREATE INDEX idx_temperature_device ON $TABLE_TEMPERATURE($COLUMN_DEVICE_ID, $COLUMN_TIME)"

        private const val SQL_CREATE_INDEX_ALARM_DEVICE =
            "CREATE INDEX idx_alarm_device ON $TABLE_ALARM_EVENTS($COLUMN_DEVICE_ID, $COLUMN_TIME)"
    }

    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL(SQL_CREATE_TABLE_TEMPERATURE)
        db.execSQL(SQL_CREATE_TABLE_ALARM_EVENTS)
        db.execSQL(SQL_CREATE_INDEX_TEMPERATURE_DEVICE)
        db.execSQL(SQL_CREATE_INDEX_ALARM_DEVICE)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        if (oldVersion < 2) {
            // v1 -> v2: 把 temperatureA 改名成 temperature,加 device_id 列并回填
            // 老 temperatureB / temperatureC 是空 stub,直接丢弃
            try {
                // 清掉残留(理论上 v1 没这些)
                db.execSQL("DROP TABLE IF EXISTS $TABLE_TEMPERATURE")
                db.execSQL("DROP TABLE IF EXISTS $TABLE_ALARM_EVENTS")

                // 把 temperatureA 改名成 temperature
                db.execSQL("ALTER TABLE temperatureA RENAME TO $TABLE_TEMPERATURE")

                // 加 device_id 列
                db.execSQL("ALTER TABLE $TABLE_TEMPERATURE ADD COLUMN $COLUMN_DEVICE_ID TEXT")

                // 回填 device_id - 优先用 MacIdBook 第一条,否则 "UNKNOWN"
                val defaultDevId = try {
                    MacIdBook.all(appContext).firstOrNull()?.first ?: "UNKNOWN"
                } catch (e: Exception) {
                    Log.w("TempDb", "MacIdBook read failed during migration: ${e.message}")
                    "UNKNOWN"
                }
                db.execSQL(
                    "UPDATE $TABLE_TEMPERATURE SET $COLUMN_DEVICE_ID = ?",
                    arrayOf(defaultDevId)
                )

                // 丢弃空表 B/C
                db.execSQL("DROP TABLE IF EXISTS temperatureB")
                db.execSQL("DROP TABLE IF EXISTS temperatureC")

                // 建 alarm_events 表 + 索引
                db.execSQL(SQL_CREATE_TABLE_ALARM_EVENTS)
                db.execSQL(SQL_CREATE_INDEX_TEMPERATURE_DEVICE)
                db.execSQL(SQL_CREATE_INDEX_ALARM_DEVICE)

                Log.i("TempDb", "Migration v1->v2 done, backfilled device_id=$defaultDevId")
            } catch (e: Exception) {
                Log.e("TempDb", "Migration v1->v2 failed, falling back to recreate: ${e.message}", e)
                // 兜底:重建(会丢老数据,但避免崩溃)
                db.execSQL("DROP TABLE IF EXISTS $TABLE_TEMPERATURE")
                db.execSQL("DROP TABLE IF EXISTS $TABLE_ALARM_EVENTS")
                db.execSQL("DROP TABLE IF EXISTS temperatureA")
                db.execSQL("DROP TABLE IF EXISTS temperatureB")
                db.execSQL("DROP TABLE IF EXISTS temperatureC")
                onCreate(db)
            }
        }
    }

    // ===== 写入 =====

    /**
     * 存储温湿度数据(每个设备一条记录)
     * @param timenow 当前时间戳(Unix 秒)
     * @param devId 8 位 HEX 设备 ID(与 MacIdBook 一致)
     * @param storeflag 1=守护中才存,0=不存
     */
    fun storeTemperatureData(
        temperatureData: List<Double>,
        humidityData: List<Double>,
        timenow: Long,
        devId: String,
        storeflag: Int
    ) {
        if (storeflag != 1) return

        val database = this.writableDatabase
        database.beginTransaction()
        try {
            val sql = """INSERT OR REPLACE INTO $TABLE_TEMPERATURE
                ($COLUMN_TIME, $COLUMN_DEVICE_ID, $COLUMN_TEMPERATURE, $COLUMN_HUMIDITY)
                VALUES (?, ?, ?, ?)"""
            val statement = database.compileStatement(sql)
            val dataSize = temperatureData.size
            for (i in temperatureData.indices) {
                val time = timenow - ((dataSize - 1 - i) * 60L)  // 60 秒间隔
                statement.bindLong(1, time)
                statement.bindString(2, devId)
                statement.bindDouble(3, temperatureData[i].toDouble())
                statement.bindDouble(4, humidityData[i].toDouble())
                statement.executeInsert()
                statement.clearBindings()
            }
            database.setTransactionSuccessful()
            Log.i("TempDb", "Save finished devId=$devId count=$dataSize")
        } finally {
            database.endTransaction()
        }
    }

    /**
     * 存报警事件
     * @param time Unix 秒
     * @return 新行 ID,-1 表示失败
     */
    fun storeAlarmEvent(devId: String, type: String, message: String, time: Long): Long {
        val database = this.writableDatabase
        var rowId = -1L

        database.beginTransaction()
        try {
            val sql = """INSERT INTO $TABLE_ALARM_EVENTS
                ($COLUMN_TIME, $COLUMN_DEVICE_ID, $COLUMN_ALARM_TYPE, $COLUMN_ALARM_MESSAGE)
                VALUES (?, ?, ?, ?)"""
            val statement = database.compileStatement(sql)
            statement.bindLong(1, time)
            statement.bindString(2, devId)
            statement.bindString(3, type)
            statement.bindString(4, message)
            rowId = statement.executeInsert()
            statement.clearBindings()
            database.setTransactionSuccessful()
        } finally {
            database.endTransaction()
        }
        return rowId
    }

    // ===== 查询 =====

    /**
     * 按时间范围查某设备的温湿度(给详情页图表用)
     * 时间参数均为 Unix 秒
     */
    fun getTemperatureHumidityByTimeRange(
        devId: String,
        startTime: Long,
        endTime: Long
    ): LiveData<Pair<List<Entry>, List<Entry>>> {
        val temperatureList = mutableListOf<Entry>()
        val humidityList = mutableListOf<Entry>()
        val liveData = MutableLiveData<Pair<List<Entry>, List<Entry>>>()

        val db = this.readableDatabase
        val query = "SELECT $COLUMN_TIME, $COLUMN_TEMPERATURE, $COLUMN_HUMIDITY FROM $TABLE_TEMPERATURE " +
            "WHERE $COLUMN_DEVICE_ID = ? AND $COLUMN_TIME BETWEEN ? AND ? " +
            "ORDER BY $COLUMN_TIME ASC"
        val cursor = db.rawQuery(query, arrayOf(devId, startTime.toString(), endTime.toString()))

        cursor.use { c ->
            val timeIdx = c.getColumnIndex(COLUMN_TIME)
            val tempIdx = c.getColumnIndex(COLUMN_TEMPERATURE)
            val humiIdx = c.getColumnIndex(COLUMN_HUMIDITY)

            if (timeIdx >= 0 && tempIdx >= 0 && humiIdx >= 0) {
                if (c.moveToFirst()) {
                    do {
                        val time = c.getLong(timeIdx)
                        val temp = c.getDouble(tempIdx)
                        val humi = c.getDouble(humiIdx)
                        temperatureList.add(Entry((time * 1000).toFloat(), temp.toFloat()))
                        humidityList.add(Entry((time * 1000).toFloat(), humi.toFloat()))
                    } while (c.moveToNext())
                }
            }
        }

        liveData.value = Pair(temperatureList, humidityList)
        return liveData
    }

    /**
     * 取所有设备的最新一条数据(给卡片首屏用)
     * @return Map<devId, Triple<timeUnixSec, temp, humi>>
     */
    fun getAllDevicesLatest(): Map<String, Triple<Long, Double, Double>> {
        val result = mutableMapOf<String, Triple<Long, Double, Double>>()
        val db = this.readableDatabase
        // 子查询:每个 device_id 取 time 最大那条
        val query = "SELECT t.$COLUMN_DEVICE_ID, t.$COLUMN_TIME, t.$COLUMN_TEMPERATURE, t.$COLUMN_HUMIDITY " +
            "FROM $TABLE_TEMPERATURE t " +
            "INNER JOIN (SELECT $COLUMN_DEVICE_ID, MAX($COLUMN_TIME) AS max_time " +
            "FROM $TABLE_TEMPERATURE GROUP BY $COLUMN_DEVICE_ID) m " +
            "ON t.$COLUMN_DEVICE_ID = m.$COLUMN_DEVICE_ID AND t.$COLUMN_TIME = m.max_time"

        val cursor = db.rawQuery(query, null)
        cursor.use { c ->
            val devIdx = c.getColumnIndex(COLUMN_DEVICE_ID)
            val timeIdx = c.getColumnIndex(COLUMN_TIME)
            val tempIdx = c.getColumnIndex(COLUMN_TEMPERATURE)
            val humiIdx = c.getColumnIndex(COLUMN_HUMIDITY)

            if (devIdx >= 0 && timeIdx >= 0 && tempIdx >= 0 && humiIdx >= 0) {
                if (c.moveToFirst()) {
                    do {
                        result[c.getString(devIdx)] = Triple(
                            c.getLong(timeIdx),
                            c.getDouble(tempIdx),
                            c.getDouble(humiIdx)
                        )
                    } while (c.moveToNext())
                }
            }
        }
        return result
    }

    /**
     * 取某设备的最近 N 条温度数据(给卡片 sparkline 用)
     * 返回时间正序的 Entry 列表
     */
    fun getSparkline(devId: String, limit: Int = 30): List<Entry> {
        val result = mutableListOf<Entry>()
        val db = this.readableDatabase
        val query = "SELECT $COLUMN_TIME, $COLUMN_TEMPERATURE FROM $TABLE_TEMPERATURE " +
            "WHERE $COLUMN_DEVICE_ID = ? ORDER BY $COLUMN_TIME DESC LIMIT ?"
        val cursor = db.rawQuery(query, arrayOf(devId, limit.toString()))
        cursor.use { c ->
            val timeIdx = c.getColumnIndex(COLUMN_TIME)
            val tempIdx = c.getColumnIndex(COLUMN_TEMPERATURE)
            if (timeIdx >= 0 && tempIdx >= 0) {
                if (c.moveToFirst()) {
                    do {
                        val time = c.getLong(timeIdx)
                        val temp = c.getDouble(tempIdx)
                        result.add(Entry((time * 1000).toFloat(), temp.toFloat()))
                    } while (c.moveToNext())
                }
            }
        }
        result.reverse()  // 让最早的在前,符合图表 X 轴递增
        return result
    }

    /**
     * 取某设备时间范围内的统计(给早起报告用)
     * 时间参数均为 Unix 秒
     */
    fun getDailyStats(devId: String, dayStart: Long, dayEnd: Long): DailyStats? {
        val db = this.readableDatabase
        val query = "SELECT COUNT(*), " +
            "MIN($COLUMN_TEMPERATURE), MAX($COLUMN_TEMPERATURE), AVG($COLUMN_TEMPERATURE), " +
            "MIN($COLUMN_HUMIDITY), MAX($COLUMN_HUMIDITY), AVG($COLUMN_HUMIDITY) " +
            "FROM $TABLE_TEMPERATURE WHERE $COLUMN_DEVICE_ID = ? AND $COLUMN_TIME BETWEEN ? AND ?"

        var stats: DailyStats? = null
        val cursor = db.rawQuery(query, arrayOf(devId, dayStart.toString(), dayEnd.toString()))
        cursor.use { c ->
            if (c.moveToFirst()) {
                val count = c.getInt(0)
                if (count > 0) {
                    stats = DailyStats(
                        devId = devId,
                        sampleCount = count,
                        tempMin = c.getDouble(1),
                        tempMax = c.getDouble(2),
                        tempAvg = c.getDouble(3),
                        humiMin = c.getDouble(4),
                        humiMax = c.getDouble(5),
                        humiAvg = c.getDouble(6)
                    )
                }
            }
        }
        return stats
    }

    /**
     * 取某设备时间范围内的报警事件(给详情页 + 早起报告用)
     * 时间参数均为 Unix 秒
     */
    fun getAlarmEvents(devId: String, startTime: Long, endTime: Long): List<AlarmEvent> {
        val result = mutableListOf<AlarmEvent>()
        val db = this.readableDatabase
        val query = "SELECT $COLUMN_ALARM_ID, $COLUMN_TIME, $COLUMN_DEVICE_ID, $COLUMN_ALARM_TYPE, $COLUMN_ALARM_MESSAGE " +
            "FROM $TABLE_ALARM_EVENTS WHERE $COLUMN_DEVICE_ID = ? AND $COLUMN_TIME BETWEEN ? AND ? " +
            "ORDER BY $COLUMN_TIME ASC"
        val cursor = db.rawQuery(query, arrayOf(devId, startTime.toString(), endTime.toString()))
        cursor.use { c ->
            val idIdx = c.getColumnIndex(COLUMN_ALARM_ID)
            val timeIdx = c.getColumnIndex(COLUMN_TIME)
            val devIdx = c.getColumnIndex(COLUMN_DEVICE_ID)
            val typeIdx = c.getColumnIndex(COLUMN_ALARM_TYPE)
            val msgIdx = c.getColumnIndex(COLUMN_ALARM_MESSAGE)

            if (idIdx >= 0 && timeIdx >= 0 && devIdx >= 0 && typeIdx >= 0 && msgIdx >= 0) {
                if (c.moveToFirst()) {
                    do {
                        result.add(AlarmEvent(
                            id = c.getLong(idIdx),
                            time = c.getLong(timeIdx),
                            devId = c.getString(devIdx),
                            type = c.getString(typeIdx),
                            message = c.getString(msgIdx)
                        ))
                    } while (c.moveToNext())
                }
            }
        }
        return result
    }

    /**
     * 取所有设备时间范围内的报警事件(给早起报告用)
     */
    fun getAllAlarmEvents(startTime: Long, endTime: Long): List<AlarmEvent> {
        val result = mutableListOf<AlarmEvent>()
        val db = this.readableDatabase
        val query = "SELECT $COLUMN_ALARM_ID, $COLUMN_TIME, $COLUMN_DEVICE_ID, $COLUMN_ALARM_TYPE, $COLUMN_ALARM_MESSAGE " +
            "FROM $TABLE_ALARM_EVENTS WHERE $COLUMN_TIME BETWEEN ? AND ? " +
            "ORDER BY $COLUMN_TIME ASC"
        val cursor = db.rawQuery(query, arrayOf(startTime.toString(), endTime.toString()))
        cursor.use { c ->
            val idIdx = c.getColumnIndex(COLUMN_ALARM_ID)
            val timeIdx = c.getColumnIndex(COLUMN_TIME)
            val devIdx = c.getColumnIndex(COLUMN_DEVICE_ID)
            val typeIdx = c.getColumnIndex(COLUMN_ALARM_TYPE)
            val msgIdx = c.getColumnIndex(COLUMN_ALARM_MESSAGE)

            if (idIdx >= 0 && timeIdx >= 0 && devIdx >= 0 && typeIdx >= 0 && msgIdx >= 0) {
                if (c.moveToFirst()) {
                    do {
                        result.add(AlarmEvent(
                            id = c.getLong(idIdx),
                            time = c.getLong(timeIdx),
                            devId = c.getString(devIdx),
                            type = c.getString(typeIdx),
                            message = c.getString(msgIdx)
                        ))
                    } while (c.moveToNext())
                }
            }
        }
        return result
    }
}

object DatabaseHelperInstance {
    private lateinit var dbHelper: TemperatureDatabaseHelper

    fun getDatabaseHelper(context: Context): TemperatureDatabaseHelper {
        if (!::dbHelper.isInitialized) {
            dbHelper = TemperatureDatabaseHelper(
                context.applicationContext,
                TemperatureDatabaseHelper.DATABASE_NAME,
                null,
                TemperatureDatabaseHelper.DATABASE_VERSION
            )
        }
        return dbHelper
    }
}

// 顶层 storeTemperatureData 函数(保持与老调用方式兼容)
// 老的 dataBaseFlag 参数被 devId 替代;1.4 阶段 MqtttService 会传真实 devId,1.1 阶段临时传 "UNKNOWN"
fun storeTemperatureData(
    context: Context,
    temperatureData: List<Double>,
    humidityData: List<Double>,
    timenow: Long,
    devId: String,
    storeflag: Int
) {
    DatabaseHelperInstance.getDatabaseHelper(context)
        .storeTemperatureData(temperatureData, humidityData, timenow, devId, storeflag)
}

// 顶层存报警事件函数
fun storeAlarmEvent(
    context: Context,
    devId: String,
    type: String,
    message: String,
    time: Long
): Long {
    return DatabaseHelperInstance.getDatabaseHelper(context)
        .storeAlarmEvent(devId, type, message, time)
}
