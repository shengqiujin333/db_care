package com.jinyuni.dengbei_care.ui.dashboard

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.Spinner
import android.widget.TextView
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import com.jinyuni.dengbei_care.DeviceParamsBook
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.R
import com.jinyuni.dengbei_care.databinding.FragmentDashboardBinding

class DashboardFragment : Fragment() {

    private var _binding: FragmentDashboardBinding? = null

    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!

    private lateinit var sendButton: Button
    private lateinit var defaultButton: Button
    private lateinit var tempdrop: EditText
    private lateinit var humdrop: EditText
    private lateinit var tempmax: EditText
    private lateinit var tempmin: EditText
    private lateinit var hummax: EditText
    private lateinit var hummin: EditText
    private lateinit var guardtime: EditText
    private lateinit var delayalarm: EditText

    private lateinit var dengBeiAlarm: CheckBox
    private lateinit var yuZhiAlarm: CheckBox
    private lateinit var zhenDongAlarm: CheckBox
    private lateinit var xiangLingAlarm: CheckBox

    private lateinit var deviceSpinner: Spinner
    private lateinit var paramScopeHint: TextView

    // 当前选中的设备 ID。null 表示"全局默认值"
    private var currentDevId: String? = null

    // spinner 项数据,第一项是"全局默认值",后面是每个设备
    private data class SpinnerItem(val devId: String?, val displayName: String) {
        override fun toString(): String = displayName
    }
    private val spinnerItems = mutableListOf<SpinnerItem>()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        val dashboardViewModel =
            ViewModelProvider(this).get(DashboardViewModel::class.java)

        _binding = FragmentDashboardBinding.inflate(inflater, container, false)
        val root: View = binding.root

        sendButton = binding.confirmButton
        defaultButton = binding.cancelButton
        tempdrop = binding.tempDropThreshold
        humdrop = binding.humidityDropThreshold
        tempmax = binding.tempRangeMax
        tempmin = binding.tempRangeMin
        hummax = binding.humidityRangeMax
        hummin = binding.humidityRangeMin
        guardtime = binding.reportInterval
        delayalarm = binding.delayReport

        dengBeiAlarm = binding.breakReportCheckbox
        yuZhiAlarm = binding.overValueReportCheckbox
        zhenDongAlarm = binding.vibrationCheckbox
        xiangLingAlarm = binding.buzzerCheckbox

        deviceSpinner = binding.deviceSpinner
        paramScopeHint = binding.paramScopeHint

        setupDeviceSpinner()

        defaultButton.setOnClickListener {
            // 恢复代码内置默认值(不动当前选中的存储,只是把输入框重置为默认值)
            tempdrop.setText("1")
            humdrop.setText("4")
            tempmax.setText("35")
            tempmin.setText("29")
            hummax.setText("70")
            hummin.setText("40")
            guardtime.setText("8")
            delayalarm.setText("5")
            dengBeiAlarm.isChecked = true
            yuZhiAlarm.isChecked = true
            zhenDongAlarm.isChecked = true
            xiangLingAlarm.isChecked = true
        }

        sendButton.setOnClickListener {
            saveCurrentValues()
        }

        return root
    }

    override fun onResume() {
        super.onResume()
        // 用户可能从传感器列表加了/删了设备,刷新 spinner
        refreshDeviceSpinner()
        loadCurrentValues()
    }

    private fun setupDeviceSpinner() {
        refreshDeviceSpinner()
        deviceSpinner.onItemSelectedListener = object : android.widget.AdapterView.OnItemSelectedListener {
            override fun onItemSelected(
                parent: android.widget.AdapterView<*>?,
                view: View?,
                position: Int,
                id: Long
            ) {
                val item = spinnerItems.getOrNull(position) ?: return
                currentDevId = item.devId
                loadCurrentValues()
            }

            override fun onNothingSelected(parent: android.widget.AdapterView<*>?) {
                currentDevId = null
                loadCurrentValues()
            }
        }
    }

    private fun refreshDeviceSpinner() {
        val previouslySelected = currentDevId
        spinnerItems.clear()
        spinnerItems.add(SpinnerItem(null, "全局默认值(所有设备)"))
        MacIdBook.all(requireContext()).forEach { (id, name) ->
            spinnerItems.add(SpinnerItem(id, "$name ($id)"))
        }
        val adapter = ArrayAdapter(
            requireContext(),
            android.R.layout.simple_spinner_item,
            spinnerItems.toList()
        ).apply {
            setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        }
        deviceSpinner.adapter = adapter

        // 尝试恢复之前的选择
        val restoreIndex = spinnerItems.indexOfFirst { it.devId == previouslySelected }
        if (restoreIndex >= 0) {
            deviceSpinner.setSelection(restoreIndex)
        }
    }

    /**
     * 把当前选中设备的参数加载到输入框。
     * - currentDevId == null:读全局扁平 key(MyPrefs.tempdrop 等)
     * - currentDevId != null:读 dev_<devId>_<param>,未设置则 fallback 到全局
     */
    private fun loadCurrentValues() {
        val ctx = requireContext()
        val devId = currentDevId

        fun loadStr(param: String, default: String): String =
            if (devId == null) {
                ctx.getSharedPreferences("MyPrefs", Context.MODE_PRIVATE)
                    .getString(param, default) ?: default
            } else {
                DeviceParamsBook.getOrGlobal(ctx, devId, param, default)
            }

        tempdrop.setText(loadStr("tempdrop", "2"))
        humdrop.setText(loadStr("humdrop", "1"))
        tempmax.setText(loadStr("tempmax", "35"))
        tempmin.setText(loadStr("tempmin", "29"))
        hummax.setText(loadStr("hummax", "75"))
        hummin.setText(loadStr("hummin", "40"))
        guardtime.setText(loadStr("guardtime", "8"))
        delayalarm.setText(loadStr("delayalarm", "8"))

        dengBeiAlarm.isChecked = loadStr("dengbeialarm", "1") == "1"
        yuZhiAlarm.isChecked = loadStr("yuzhialarm", "1") == "1"
        zhenDongAlarm.isChecked = loadStr("zhendongalarm", "1") == "1"
        xiangLingAlarm.isChecked = loadStr("xianglingalarm", "1") == "1"

        // 更新提示文字
        paramScopeHint.text = if (devId == null) {
            "选「全局默认值」会应用到所有未单独设置的设备;选某设备可单独覆盖默认值"
        } else {
            val name = MacIdBook.all(ctx).find { it.first == devId }?.second ?: devId
            "正在编辑 [$name] 的独立参数。未填写的字段会回退到全局默认值"
        }
    }

    /**
     * 保存当前输入框的值到当前选中设备的存储。
     */
    private fun saveCurrentValues() {
        val tempdropdata = tempdrop.text.toString()
        val humdropdata = humdrop.text.toString()
        val tempmaxdata = tempmax.text.toString()
        val tempmindata = tempmin.text.toString()
        val hummaxdata = hummax.text.toString()
        val hummindata = hummin.text.toString()
        val guardtimedata = guardtime.text.toString()
        val delayalarmdata = delayalarm.text.toString()

        if (!isValidInput(tempdropdata) || !isValidInput(humdropdata) || !isValidInput(tempmaxdata)
            || !isValidInput(tempmindata) || !isValidInput(hummaxdata) || !isValidInput(hummindata)
            || !isValidInput(guardtimedata) || !isValidInput(delayalarmdata)) {
            Toast.makeText(requireContext(), " 输入 0 到 100的整数或者一位小数.", Toast.LENGTH_SHORT).show()
            return
        }

        val dengbeialarmflag = if (dengBeiAlarm.isChecked) "1" else "0"
        val yuzialarmflag = if (yuZhiAlarm.isChecked) "1" else "0"
        val zhendongalarmflag = if (zhenDongAlarm.isChecked) "1" else "0"
        val xianglingalarmflag = if (xiangLingAlarm.isChecked) "1" else "0"

        val ctx = requireContext()
        val devId = currentDevId
        if (devId == null) {
            // 全局模式:写扁平 key
            ctx.getSharedPreferences("MyPrefs", Context.MODE_PRIVATE).edit()
                .putString("tempdrop", tempdropdata)
                .putString("humdrop", humdropdata)
                .putString("tempmax", tempmaxdata)
                .putString("tempmin", tempmindata)
                .putString("hummax", hummaxdata)
                .putString("hummin", hummindata)
                .putString("guardtime", guardtimedata)
                .putString("delayalarm", delayalarmdata)
                .putString("dengbeialarm", dengbeialarmflag)
                .putString("yuzhialarm", yuzialarmflag)
                .putString("zhendongalarm", zhendongalarmflag)
                .putString("xianglingalarm", xianglingalarmflag)
                .apply()
            Toast.makeText(ctx, "已保存全局默认参数", Toast.LENGTH_SHORT).show()
        } else {
            // 单设备模式:写 dev_<devId>_<param>
            DeviceParamsBook.set(ctx, devId, "tempdrop", tempdropdata)
            DeviceParamsBook.set(ctx, devId, "humdrop", humdropdata)
            DeviceParamsBook.set(ctx, devId, "tempmax", tempmaxdata)
            DeviceParamsBook.set(ctx, devId, "tempmin", tempmindata)
            DeviceParamsBook.set(ctx, devId, "hummax", hummaxdata)
            DeviceParamsBook.set(ctx, devId, "hummin", hummindata)
            DeviceParamsBook.set(ctx, devId, "guardtime", guardtimedata)
            DeviceParamsBook.set(ctx, devId, "delayalarm", delayalarmdata)
            DeviceParamsBook.set(ctx, devId, "dengbeialarm", dengbeialarmflag)
            DeviceParamsBook.set(ctx, devId, "yuzhialarm", yuzialarmflag)
            DeviceParamsBook.set(ctx, devId, "zhendongalarm", zhendongalarmflag)
            DeviceParamsBook.set(ctx, devId, "xianglingalarm", xianglingalarmflag)
            val name = MacIdBook.all(ctx).find { it.first == devId }?.second ?: devId
            Toast.makeText(ctx, "已保存 [$name] 的独立参数", Toast.LENGTH_SHORT).show()
        }
    }

    private fun isValidInput(input: String): Boolean {
        return input.matches(Regex("^\\d+(\\.\\d{1,2})?$")) && input.toDoubleOrNull()?.let { it in 0.0..100.0 } ?: false
    }


    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
