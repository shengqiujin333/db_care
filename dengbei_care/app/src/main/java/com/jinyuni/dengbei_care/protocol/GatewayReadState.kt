package com.jinyuni.dengbei_care.protocol

/**
 * BLE 读取尝试的终态链路状态（AA-006 N-2 / O-AA005-6）。
 *
 * 纯逻辑：每次读取尝试（成功、占位、拒绝、长度不足）都必须落到一个可区分终态，
 * 不允许首页链路条残留在 `CONNECTING`。状态码稳定供测试与日志断言；文案仅为类别，
 * 具体时间等运行时信息由调用方传入。
 */
object GatewayReadState {

    enum class Code {
        /** 有效读数（至少一条非占位） */
        DATA_OK,

        /** 完整快照但全部为占位块 */
        GATEWAY_NO_DATA,

        /** devCount=0 / 网关无设备 */
        GATEWAY_NO_DEVICE,

        /** 密文/明文长度不足或未覆盖完整快照（含 MTU 不足） */
        READ_INCOMPLETE,

        /** 帧头/解密/密文对齐等无效数据 */
        READ_INVALID
    }

    data class Status(val code: Code, val text: String)

    /** 成功解码后的终态；[timeLabel] 例如 "21:57"。 */
    fun success(timeLabel: String, devCount: Int, validCount: Int): Status = when {
        devCount <= 0 -> Status(Code.GATEWAY_NO_DEVICE, "网关已连接，暂无设备（N=0）")
        validCount <= 0 -> Status(Code.GATEWAY_NO_DATA, "网关暂无传感器数据（等待传感器上报）")
        else -> Status(Code.DATA_OK, "网关已连接（最近读取 $timeLabel）")
    }

    /** 解密/解码拒绝的终态。 */
    fun rejected(
        reason: GatewayFrameCodec.RejectionReason,
        detail: String = ""
    ): Status {
        val base = when (reason) {
            GatewayFrameCodec.RejectionReason.NO_DEVICE ->
                Status(Code.GATEWAY_NO_DEVICE, "网关已连接，暂无设备（N=0）")
            GatewayFrameCodec.RejectionReason.LENGTH_MISMATCH ->
                Status(Code.READ_INCOMPLETE, "读取不完整（长度不足）")
            GatewayFrameCodec.RejectionReason.FRAME_TOO_SHORT ->
                Status(Code.READ_INVALID, "网关已连接，数据无效（帧头不足）")
            GatewayFrameCodec.RejectionReason.DECRYPT_FAILED ->
                Status(Code.READ_INVALID, "网关已连接，数据无效（解密失败）")
            GatewayFrameCodec.RejectionReason.CIPHERTEXT_NOT_ALIGNED ->
                Status(Code.READ_INVALID, "网关已连接，数据无效（密文未对齐）")
        }
        // READ_INVALID 保留原因，便于现场区分帧头/解密/对齐问题
        return if (base.code == Code.READ_INVALID && detail.isNotBlank()) {
            base.copy(text = base.text + "：" + detail)
        } else {
            base
        }
    }

    /** 长度守卫拒绝的终态（MTU/返回字节不足、非 16 对齐）。 */
    fun lengthRefused(
        reason: GatewayReadGuard.ReadRefuseReason,
        actualBytes: Int,
        expectedBytes: Int
    ): Status = when (reason) {
        GatewayReadGuard.ReadRefuseReason.MTU_TOO_SMALL ->
            Status(Code.READ_INCOMPLETE, "读取不完整（MTU 不足）")
        GatewayReadGuard.ReadRefuseReason.LENGTH_TOO_SMALL ->
            Status(
                Code.READ_INCOMPLETE,
                "读取不完整（返回 $actualBytes 字节 / 需要 $expectedBytes 字节）"
            )
        GatewayReadGuard.ReadRefuseReason.LENGTH_NOT_ALIGNED ->
            Status(Code.READ_INCOMPLETE, "读取不完整（返回 $actualBytes 字节，非 16 字节对齐）")
    }

    /** 读取回调直接失败（无返回字节/非 16 对齐）的终态。 */
    fun unaligned(returnedBytes: Int): Status =
        Status(Code.READ_INCOMPLETE, "读取不完整（返回 $returnedBytes 字节，非 16 字节对齐）")
}
