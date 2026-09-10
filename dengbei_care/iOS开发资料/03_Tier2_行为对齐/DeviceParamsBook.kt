package com.jinyuni.dengbei_care

import android.content.Context
import android.content.SharedPreferences

/**
 * 管理每设备独立的报警参数。
 *
 * 存储格式:`dev_<devId>_<paramName>` -> 字符串值
 *
 * 如果某设备没有为某参数设值,返回 null - 调用方应 fallback 到全局默认值
 * (DashboardFragment 存的扁平 key)。
 *
 * 参数维度与 DashboardFragment 全局参数一一对应:
 * - tempdrop / humdrop    下降报警阈值
 * - tempmax / tempmin     温度阈值上下限
 * - hummax / hummin       湿度阈值上下限
 * - guardtime             守护时长(小时)
 * - delayalarm            延时报警(秒)
 * - dengbeialarm          下降报警开关 (0/1)
 * - yuzhialarm            阈值报警开关 (0/1)
 * - zhendongalarm         震动报警开关 (0/1)
 * - xianglingalarm        蜂鸣报警开关 (0/1)
 */
object DeviceParamsBook {

    private const val PREFS_NAME = "MyPrefs"
    private const val KEY_PREFIX = "dev_"

    /** 取某设备的某参数。未设置返回 null。 */
    fun get(context: Context, devId: String, param: String): String? {
        val prefs = prefs(context)
        val key = makeKey(devId, param)
        val v = prefs.getString(key, null)
        return v?.takeIf { it.isNotBlank() }
    }

    /** 取某设备的某参数。未设置返回 fallback。 */
    fun getOr(context: Context, devId: String, param: String, fallback: String): String =
        get(context, devId, param) ?: fallback

    /** 取某设备的某参数。未设置时取全局值,全局也没有时取 fallback。 */
    fun getOrGlobal(context: Context, devId: String, param: String, fallback: String): String {
        get(context, devId, param)?.let { return it }
        val prefs = prefs(context)
        val globalValue = prefs.getString(param, null)
        return globalValue?.takeIf { it.isNotBlank() } ?: fallback
    }

    /** 写入某设备的某参数。 */
    fun set(context: Context, devId: String, param: String, value: String) {
        prefs(context).edit().putString(makeKey(devId, param), value).apply()
    }

    /** 删除某设备的所有参数(从 MacIdBook 删除设备时调用)。 */
    fun clearDevice(context: Context, devId: String) {
        val prefs = prefs(context)
        val editor = prefs.edit()
        val all = prefs.all
        val prefix = "${KEY_PREFIX}${devId}_"
        all.keys.filter { it.startsWith(prefix) }.forEach { editor.remove(it) }
        editor.apply()
    }

    /** 该设备是否已经设过任何参数(用于判断是否需要从全局默认初始化)。 */
    fun hasAnyParams(context: Context, devId: String): Boolean {
        val prefix = "${KEY_PREFIX}${devId}_"
        return prefs(context).all.keys.any { it.startsWith(prefix) }
    }

    /**
     * 用一组默认值初始化某设备的参数。如果该设备已存在任何参数,跳过。
     * 用于新设备首次添加时,把当前 DashboardFragment 的全局值 copy 一份过去。
     */
    fun initFromGlobalIfAbsent(context: Context, devId: String) {
        if (hasAnyParams(context, devId)) return
        val prefs = prefs(context)
        val editor = prefs.edit()
        PARAMS.forEach { param ->
            val globalValue = prefs.getString(param, null)
            if (globalValue != null) {
                editor.putString(makeKey(devId, param), globalValue)
            }
        }
        editor.apply()
    }

    private fun makeKey(devId: String, param: String): String = "${KEY_PREFIX}${devId}_$param"

    private fun prefs(context: Context): SharedPreferences =
        context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    /** 所有支持的参数名(用于初始化/清理)。 */
    val PARAMS = listOf(
        "tempdrop", "humdrop",
        "tempmax", "tempmin", "hummax", "hummin",
        "guardtime", "delayalarm",
        "dengbeialarm", "yuzhialarm",
        "zhendongalarm", "xianglingalarm"
    )
}
