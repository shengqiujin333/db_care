package com.jinyuni.dengbei_care.ui.notifications

import android.app.Application
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider

class NotificationsViewModel private constructor(): ViewModel() {

    companion object {
        private var instance: NotificationsViewModel? = null

        fun getInstance(application: Application): NotificationsViewModel {
            if (instance == null) {
                instance = ViewModelProvider.AndroidViewModelFactory.getInstance(application).create(
                    NotificationsViewModel::class.java)
            }
            return instance!!
        }
    }

    val _mqtt_broker_state = MutableLiveData<String>()
    val mqtt_broker_state: LiveData<String> = _mqtt_broker_state

    private val _text = MutableLiveData<String>().apply {
        value = "This is notifications Fragment"
    }
    val text: LiveData<String> = _text

    fun updateMqttBrokerStateData(data: String) {
        _mqtt_broker_state.postValue(data)
    }
}