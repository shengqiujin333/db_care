package com.jinyuni.dengbei_care.ui.zhuce

import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.Headers
import retrofit2.http.POST

interface ApiService {
    @Headers("Content-Type: application/json")
    @POST("/sendVerificationCode")
    suspend fun sendVerificationCode(@Body phoneNumber: VerificationRequest): Response<Void>

    @POST("/verifyCode")
    suspend fun verifyCode(@Body verificationData: VerificationData): Response<Void>

    @POST("/register")
    suspend fun register(@Body registrationData: RegistrationData): Response<Void>

    @POST("/login")
    suspend fun login(@Body loginData: LoginData): Response<LoginResponse>

    @POST("/setmac")
    suspend fun setMacAddress(@Body macData: MacData): Response<Void>

    /**
     * 传感器温湿度数据上传（服务器迁移记录 §八）。
     * 幂等由服务端 `UNIQUE(dev_id, time)` + `INSERT IGNORE` 保证，App 侧由待发箱 (dev_id, time) 去重。
     */
    @Headers("Content-Type: application/json")
    @POST("/upload_data")
    suspend fun uploadData(@Body data: SensorUploadData): Response<Void>
}

data class VerificationRequest(val phoneNumber: String)
data class VerificationData(val phoneNumber: String, val code: String)
data class RegistrationData(val phoneNumber: String, val password: String)
data class LoginData(val phoneNumber: String, val password: String)
data class LoginResponse(val success: Boolean, val token: String?)
data class MacData(val phoneNumber: String, val macAddress: String)

/** 单条传感器读数（字段名与 `/upload_data` 契约一致） */
data class SensorReading(
    val devId: String,        // 8 位 HEX 设备 ID（来自记录自身归因）
    val time: Long,           // Unix 秒（真实采样时间）
    val temperature: Double,  // 摄氏度
    val humidity: Double      // 百分比相对湿度 %RH
)

/** `/upload_data` 请求体：phone、mac、readings[] */
data class SensorUploadData(
    val phone: String,               // 归属用户手机号（registerphone）
    val mac: String,                 // 网关 MAC（mac_addr）
    val readings: List<SensorReading>
)