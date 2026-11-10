package com.jinyuni.dengbei_care.cloud

/**
 * 云上传结果分类（AA-005 G-6 / D-27 / D-28）。
 *
 * 纯逻辑：
 * - `Success`：HTTP 2xx（仅表示服务端受理，不代表已持久化）；
 * - `AuthRejected`：401/403 → 保留数据、暂停自动重试、可观测、可手动重试；
 * - `RequestRejected`：其余 4xx（408/429 除外）→ 同上；
 * - `Retryable`：5xx、408、429、超时/IO/解析异常 → 沿用既有退避。
 *
 * 只按 HTTP 状态与异常类型分类，不解析未证实的响应体，不新增鉴权协议。
 */
sealed interface UploadResult {

    data object Success : UploadResult

    data class AuthRejected(val httpStatus: Int) : UploadResult

    data class RequestRejected(val httpStatus: Int) : UploadResult

    data class Retryable(val httpStatus: Int?, val cause: String) : UploadResult

    companion object {
        fun fromHttp(status: Int?): UploadResult = when {
            status == null -> Retryable(null, "no_http_status")
            status in 200..299 -> Success
            status == 401 || status == 403 -> AuthRejected(status)
            status == 408 || status == 429 -> Retryable(status, "http_$status")
            status in 400..499 -> RequestRejected(status)
            status in 500..599 -> Retryable(status, "http_$status")
            else -> Retryable(status, "unexpected_http_$status")
        }

        fun fromError(cause: Throwable): UploadResult =
            Retryable(null, cause.javaClass.simpleName + ": " + (cause.message ?: ""))

        fun fromError(cause: String): UploadResult = Retryable(null, cause)
    }
}
