package com.jinyuni.dengbei_care
// MacIdBook.kt

import android.content.Context
import android.content.SharedPreferences
import org.json.JSONObject

/**
 * 管理 (id, name)：
 * - id：12位十六进制（不带0x/分隔符），持久化为大写（如 A1B2C3D4E5F6）
 * - name：中文 ≤3 个汉字；或英数 1..12 个字符（全部转小写）；不允许符号/空格/中英混合
 * - 只能新增/删除/读取/计数（不修改）
 * - 存储在 MyPrefs 的 JSON 映射里（key: "macid_name_map"）
 */
object MacIdBook {

    private const val PREFS_NAME = "MyPrefs"
    private const val KEY_MAP    = "macid_name_map"

    // ---------- 对外 API ----------

    /** 新增 (id,name)。返回 true 表示新增成功；false 为校验失败或 id 已存在。 */
    @Synchronized
    fun add(context: Context, idRaw: String, nameRaw: String, immediate: Boolean = false): Boolean {
        val id = sanitizeId12(idRaw) ?: return false
        val name = sanitizeName(nameRaw) ?: return false

        val prefs = prefs(context)
        val map = loadMap(prefs)
        if (map.containsKey(id)) return false

        map[id] = name
        return saveMap(prefs, map, immediate)
    }

    /** 删除（按 id）。成功返回 true。 */
    @Synchronized
    fun remove(context: Context, idRaw: String, immediate: Boolean = false): Boolean {
        val id = sanitizeId12(idRaw) ?: return false
        val prefs = prefs(context)
        val map = loadMap(prefs)
        val removed = (map.remove(id) != null)
        if (!removed) return false
        return saveMap(prefs, map, immediate)
    }

    /** 清空全部。 */
    @Synchronized
    fun clear(context: Context, immediate: Boolean = false): Boolean {
        val prefs = prefs(context)
        return saveMap(prefs, mutableMapOf(), immediate)
    }

    /** 读取所有 (id,name)，按 name → id 排序。 */
    fun all(context: Context): List<Pair<String, String>> {
        val map = loadMap(prefs(context))
        return map.entries
            .sortedWith(compareBy<Map.Entry<String, String>> { it.value }.thenBy { it.key })
            .map { it.key to it.value }
    }

    /** 数量 */
    fun count(context: Context): Int = loadMap(prefs(context)).size

    /** 是否存在某 id */
    fun contains(context: Context, idRaw: String): Boolean {
        val id = sanitizeId12(idRaw) ?: return false
        return loadMap(prefs(context)).containsKey(id)
    }

    // ---------- 规则校验 ----------

    /** id：12位HEX，转大写、无分隔符 */
    private fun sanitizeId12(input: String): String? {
        val s = input.trim()                       // 仅去掉首尾空白
        if (s.length != 8) return null            // 长度必须正好 12
        if (!s.matches(Regex("^[0-9A-Fa-f]{8}$"))) return null  // 只允许十六进制字符
        return s.uppercase()                       // 统一大写后存储
    }

    /**
     * name 规则：
     * - 有汉字：必须全是汉字，且 ≤3 个
     * - 无汉字：只允许 [a-z0-9]，长度 1..12，统一转小写
     * - 不允许符号/空格/中英混合
     */
    private fun sanitizeName(raw: String): String? {
        val s = raw.trim()
        if (s.isEmpty()) return null

        val hasHan = s.any { it.isHan() }
        val hasNonHan = s.any { !it.isHan() }

        return if (hasHan) {
            if (hasNonHan) return null
            if (s.length > 3) return null
            s
        } else {
            val lowered = s.lowercase()
            if (!lowered.matches(Regex("^[a-z0-9]{1,8}$"))) return null
            lowered
        }
    }

    private fun Char.isHan(): Boolean {
        val code = this.code
        return (code in 0x4E00..0x9FFF) || (code in 0x3400..0x4DBF)
    }

    // ---------- 存取底层 ----------

    private fun prefs(context: Context): SharedPreferences =
        context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    private fun loadMap(prefs: SharedPreferences): MutableMap<String, String> {
        val json = prefs.getString(KEY_MAP, "{}") ?: "{}"
        val obj = try { JSONObject(json) } catch (_: Throwable) { JSONObject("{}") }
        val map = mutableMapOf<String, String>()
        val it = obj.keys()
        while (it.hasNext()) {
            val id = it.next()
            val name = obj.optString(id, null) ?: continue
            map[id] = name
        }
        return map
    }

    private fun saveMap(
        prefs: SharedPreferences,
        map: MutableMap<String, String>,
        immediate: Boolean
    ): Boolean {
        val obj = JSONObject()
        map.forEach { (id, name) -> obj.put(id, name) }
        val editor = prefs.edit().putString(KEY_MAP, obj.toString())
        return if (immediate) editor.commit() else { editor.apply(); true }
    }
}
