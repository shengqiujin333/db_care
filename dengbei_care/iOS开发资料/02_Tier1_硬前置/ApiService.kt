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
}

data class VerificationRequest(val phoneNumber: String)
data class VerificationData(val phoneNumber: String, val code: String)
data class RegistrationData(val phoneNumber: String, val password: String)
data class LoginData(val phoneNumber: String, val password: String)
data class LoginResponse(val success: Boolean, val token: String?)
data class MacData(val phoneNumber: String, val macAddress: String)