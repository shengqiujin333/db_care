package com.jinyuni.dengbei_care.ui.zhuce

import android.util.Log
import androidx.lifecycle.ViewModel
import androidx.lifecycle.liveData
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory

class ZhuCeViewModel: ViewModel() {
    private val apiService = Retrofit.Builder()
        .baseUrl("http://117.72.84.210:5000")  // 替换为服务器地址
        .addConverterFactory(GsonConverterFactory.create())
        .build()
        .create(ApiService::class.java)

    fun sendVerificationCode(phoneNumber: String) = liveData {
        try {
            val response = apiService.sendVerificationCode(VerificationRequest(phoneNumber))
            emit(response.isSuccessful)
        } catch (e: Exception) {
            emit(false)
            Log.e("ZhuCeViewModel", "Error sending verification code: ${e.message}")
        }
    }

    fun verifyCode(phoneNumber: String, code: String) = liveData {
        try {
            val response = apiService.verifyCode(VerificationData(phoneNumber, code))
            emit(response.isSuccessful)
        } catch (e: Exception) {
            emit(false)
        }
    }

    fun register(phoneNumber: String, password: String) = liveData {
        try {
            val response = apiService.register(RegistrationData(phoneNumber, password))
            emit(response.isSuccessful)
        } catch (e: Exception) {
            emit(false)
        }
    }

    fun login(phoneNumber: String, password: String) = liveData {
        try {
            val response = apiService.login(LoginData(phoneNumber, password))
            emit(response.isSuccessful)
        } catch (e: Exception) {
            emit(false)
        }
    }

    fun setMacAddress(phoneNumber: String, macAddress: String) = liveData {
        try {
            val response = apiService.setMacAddress(MacData(phoneNumber, macAddress))
            emit(response.isSuccessful)
        } catch (e: Exception) {
            emit(false)
        }
    }

}