package com.jinyuni.dengbei_care.ui.sensorid

import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.recyclerview.widget.RecyclerView
import com.jinyuni.dengbei_care.DeviceParamsBook
import com.jinyuni.dengbei_care.MacIdBook
import com.jinyuni.dengbei_care.R
import com.jinyuni.dengbei_care.SensorIdAdapter
class SensorIdListFragment : Fragment(R.layout.fragment_sensor_id_list) {

    private lateinit var recycler: RecyclerView
    private lateinit var btnDeleteSelected: Button
    private lateinit var tvTitle: TextView
    private lateinit var adapter: SensorIdAdapter

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        recycler = view.findViewById(R.id.recyclerSensorIds)
        btnDeleteSelected = view.findViewById(R.id.btnDeleteSelected)
        tvTitle = view.findViewById(R.id.titleSavedIds)

        adapter = SensorIdAdapter(
            onDeleteSingle = { id ->
                MacIdBook.remove(requireContext(), id, immediate = true)
                DeviceParamsBook.clearDevice(requireContext(), id)
                refresh()
            },
            onSelectionChanged = { selected ->
                btnDeleteSelected.isEnabled = selected.isNotEmpty()
            }
        )
        recycler.adapter = adapter

        btnDeleteSelected.setOnClickListener {
            val toDelete = adapter.getSelected().toList()
            toDelete.forEach { id ->
                MacIdBook.remove(requireContext(), id, immediate = true)
                DeviceParamsBook.clearDevice(requireContext(), id)
            }
            refresh()
        }



        refresh()
    }

    private fun refresh() {
        val list = MacIdBook.all(requireContext()) // List<Pair<id, name>>
        adapter.submitList(list)
        tvTitle.text = getString(R.string.saved_ids_title) + "（${list.size}）"
    }
}
