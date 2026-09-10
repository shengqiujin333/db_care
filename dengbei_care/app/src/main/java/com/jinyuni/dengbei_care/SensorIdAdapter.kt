package com.jinyuni.dengbei_care

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.CheckBox
import android.widget.ImageButton
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView

class SensorIdAdapter(
    private val onDeleteSingle: (id: String) -> Unit,
    private val onSelectionChanged: (selected: Set<String>) -> Unit
) : RecyclerView.Adapter<SensorIdAdapter.VH>() {

    data class Row(val id: String, val name: String)

    private val data = mutableListOf<Row>()
    private val selected = mutableSetOf<String>() // 只存 id

    fun submitList(pairs: List<Pair<String, String>>) {
        data.clear()
        data.addAll(pairs.map { Row(id = it.first, name = it.second) })
        // 清理失效选择
        val allIds = data.map { it.id }.toSet()
        selected.retainAll(allIds)
        notifyDataSetChanged()
        onSelectionChanged(selected)
    }

    fun getSelected(): Set<String> = selected

    inner class VH(view: View) : RecyclerView.ViewHolder(view) {
        val cb: CheckBox = view.findViewById(R.id.cbSelect)
        val tv: TextView = view.findViewById(R.id.tvNameAndId)
        val btnDel: ImageButton = view.findViewById(R.id.btnDelete)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VH {
        val v = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_sensor_id, parent, false)
        return VH(v)
    }

    override fun onBindViewHolder(holder: VH, position: Int) {
        val row = data[position]
        // 显示：别称 + 空格 + id（id 为 12 位 HEX，无任何符号）
        holder.tv.text = "${row.name} ${row.id}"

        // 选中状态
        holder.cb.setOnCheckedChangeListener(null)
        holder.cb.isChecked = selected.contains(row.id)
        holder.cb.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked) selected.add(row.id) else selected.remove(row.id)
            onSelectionChanged(selected)
        }

        // 点击整行切换勾选
        holder.itemView.setOnClickListener {
            holder.cb.isChecked = !holder.cb.isChecked
        }

        // 单行删除
        holder.btnDel.setOnClickListener {
            onDeleteSingle(row.id)
        }
    }

    override fun getItemCount(): Int = data.size
}
