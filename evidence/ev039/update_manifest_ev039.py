import io

p = 'file_manifest.txt'
s = io.open(p, encoding='utf-8').read()
lines = s.split('\n')

entries = [
    ("evidence/ev039/", "active", "EV-039 当前固件独立实板验证原始证据目录：独立重建/下载、端口先开后 pyocd HW reset 的上电五行、4 份 COM43 原始捕获（6 个采样行、4 连续周期）、独立分析/静态/宿主 harness 日志与工具原始结果；含人工 SKIP_CONFIRMED / E2E-04 DEFERABLE 披露"),
    ("evidence/ev039/EV-039_section.md", "active", "EV-039 证据正文（已并入 evidence/test.md 顶部）：身份/重建下载/宿主 222+609/0/静态 21/21/上电五行/COM43 4 连续周期/可选 L3 未执行/延期与阶段外未执行披露/结论 TEST_PASS（限定 TD-002 rev 9.0 适用范围内）"),
    ("evidence/ev039/EV-039_prototype_section.md", "active", "EV-039 下载身份段落（已并入 evidence/programmed_prototype.md 顶部）"),
    ("evidence/ev039/tool_results.txt", "active", "本轮端口/USB/sigrok 枚举、构建与下载身份、宿主与静态计数、BOT 与 COM43 工具原始返回、pyocd 参数与未执行项"),
    ("evidence/ev039/capture_windows.txt", "active", "4 个捕获窗口起止/字节数/md5/观测 k/窗口间隙与连续集说明（k=6 落在工具间隙）"),
    ("evidence/ev039/capture_analysis.txt", "active", "analyze_ev039.py 输出（104 项断言 0 失败：行格式/字段/节拍/p 链/无杂散/判据未启用等价性）"),
    ("evidence/ev039/bot01_hw.cap", "active", "BOT-01 原始字节 271 B（BOOT/VDD/IOTEST/IOSIG/BUS + S k=0；md5 3cbb7c68…）"),
    ("evidence/ev039/cap01_k3.cap", "active", "COM43 被动原始字节：S k=3，78 B（md5 d025737c…）"),
    ("evidence/ev039/cap02_k9_k12.cap", "active", "COM43 被动原始字节：S k=9/k=12，157 B（md5 2cbd9e24…）"),
    ("evidence/ev039/cap03_k15_k18.cap", "active", "COM43 被动原始字节：S k=15/k=18，158 B（md5 c6cd6b77…）"),
    ("evidence/ev039/bot01_run.txt", "active", "端口先开/复位时间/pyocd 原始输出/逐块接收的 BOT-01 原始运行日志"),
    ("evidence/ev039/mdk_build_flash.log", "active", "mdk_build rebuild 与 mdk_flash 原始日志（0E/1W、Code=25964、axf b6a15fed…、Verify OK/Application running）"),
    ("evidence/ev039/host_build_test.log", "active", "实现侧 test/build_test.sh 原始输出（48+61+54+28+31=222 项 0 失败）"),
    ("evidence/ev039/host_harness_run.stdout", "active", "测试侧独立 harness 复跑 stdout（13 组 609/0；RESULT: FAILED=[]）"),
    ("evidence/ev039/host_fw_core_verify_ev.build.log", "active", "fw_core 独立 harness 编译日志（空=0 error）"),
    ("evidence/ev039/host_fw_core_verify_ev.log", "active", "fw_core 独立 harness 输出（59/0）"),
    ("evidence/ev039/host_gxht40_verify_ev.build.log", "active", "GXHT40 独立 harness 编译日志"),
    ("evidence/ev039/host_gxht40_verify_ev.log", "active", "GXHT40 独立 harness 输出（93/0）"),
    ("evidence/ev039/host_sf_i2c_verify_ev.build.log", "active", "软 I2C 独立 harness 编译日志"),
    ("evidence/ev039/host_sf_i2c_verify_ev.log", "active", "软 I2C 独立 harness 输出（51/0）"),
    ("evidence/ev039/host_diag_line_verify_ev.build.log", "active", "诊断行独立 harness 编译日志"),
    ("evidence/ev039/host_diag_line_verify_ev.log", "active", "诊断行独立 harness 输出（38/0）"),
    ("evidence/ev039/host_diag_probe_verify_ev.build.log", "active", "总线诊断独立 harness 编译日志"),
    ("evidence/ev039/host_diag_probe_verify_ev.log", "active", "总线诊断独立 harness 输出（109/0）"),
    ("evidence/ev039/host_measure_flow_verify_ev.build.log", "active", "采样流程独立 harness 编译日志"),
    ("evidence/ev039/host_measure_flow_verify_ev.log", "active", "采样流程独立 harness 输出（79/0）"),
    ("evidence/ev039/host_rf_frame_verify_ev.build.log", "active", "空口帧独立 harness 编译日志"),
    ("evidence/ev039/host_rf_frame_verify_ev.log", "active", "空口帧独立 harness 输出（10/0）"),
    ("evidence/ev039/host_rf_report_verify_ev.build.log", "active", "条件上报发送路径独立 harness 编译日志"),
    ("evidence/ev039/host_rf_report_verify_ev.log", "active", "条件上报发送路径独立 harness 输出（30/0）"),
    ("evidence/ev039/host_rtc_cadence_verify_ev.main.build.log", "active", "RTC 节拍 harness 的 main.c 影子构建日志"),
    ("evidence/ev039/host_rtc_cadence_verify_ev.harness.build.log", "active", "RTC 节拍 harness 编译日志"),
    ("evidence/ev039/host_rtc_cadence_verify_ev.link.log", "active", "RTC 节拍 harness 链接日志"),
    ("evidence/ev039/host_rtc_cadence_verify_ev.log", "active", "RTC 节拍独立 harness 输出（17/0）"),
    ("evidence/ev039/host_gw_compat_verify_ev.build.log", "active", "网关兼容独立 harness 编译日志"),
    ("evidence/ev039/host_gw_compat_verify_ev.log", "active", "网关兼容独立 harness 输出（11/0）"),
    ("evidence/ev039/host_vdd_verify_ev.build.log", "active", "供电轨测量独立 harness 编译日志"),
    ("evidence/ev039/host_vdd_verify_ev.log", "active", "供电轨测量独立 harness 输出（31/0）"),
    ("evidence/ev039/host_light_verify_ev.build.log", "active", "light.c（T=0）独立 harness 编译日志"),
    ("evidence/ev039/host_light_verify_ev.log", "active", "light.c（T=0）独立 harness 输出（40/0）"),
    ("evidence/ev039/host_light_verify_ev_enabled.build.log", "active", "light.c（T=3000）独立 harness 编译日志"),
    ("evidence/ev039/host_light_verify_ev_enabled.log", "active", "light.c（T=3000）独立 harness 输出（41/0）"),
    ("evidence/ev039/static_check_ev039.txt", "active", "只读静态核查输出（21/21 PASS：常量/判据/行序/上电顺序/引脚/无浮点/无可疑旧通路/有界等待）"),
    ("evidence/ev039/build_debug_off.log", "active", "SENSOR_DEBUG_UART=0 构建与符号检查原始输出（8 个翻译单元 exit 0；无 UART 符号）"),
    ("evidence/ev039/analyze_ev039.py", "active", "本轮独立解析脚本（行字段/行长/CRLF/杂散/节拍/p 链/可选 I2C 段；不引用实现常量）"),
    ("evidence/ev039/capture_with_reset.py", "active", "BOT-01 编排脚本（COM43 先开->pyocd 通用 cortex_m HW reset->有界采集；仅复位，不写 Flash/内存）"),
    ("evidence/ev039/run_host_harnesses.sh", "active", "测试侧独立 harness 构建/运行脚本（13 组）"),
    ("evidence/ev039/static_check_ev039.py", "active", "只读静态核查脚本（21 项；可复跑）"),
    ("evidence/ev039/build_debug_off.sh", "active", "SENSOR_DEBUG_UART=0 构建与符号检查脚本（可复跑）"),
    ("evidence/ev039/update_manifest_ev039.py", "active", "本轮清单更新脚本（新增 EV-039 条目并刷新聚合证据描述；可复跑）"),
]

anchor = 'evidence/ev038/tool_results.txt | active |'
idx = None
for i, l in enumerate(lines):
    if l.startswith(anchor):
        idx = i
        break
if idx is None:
    raise SystemExit('anchor not found')
if any(l.startswith('evidence/ev039/') for l in lines):
    raise SystemExit('ev039 entries already present')

lines[idx + 1:idx + 1] = ['%s | %s | %s' % e for e in entries]

new_test = ('evidence/test.md | active | 嵌入式验证证据 EV-039（当前轮，置顶）：TD-002 rev 9.0 适用范围内 TEST_PASS（限定已执行范围）'
            '— 产品源 82a7e74 零差异、重建 axf b6a15fed…/hex 4d728dec… 与发布包成员一致、mdk_flash Verify OK、'
            '端口先开后 pyocd HW reset 取上电五行与 S k=0、COM43 6 个采样行 q=1（k=9/12/15/18 连续）且 p 链匹配、'
            '宿主 222+609/0、静态 21/21、SENSOR_DEBUG_UART=0 无 UART 符号；'
            'FW-LIGHT-DEF2-ENABLE 为 SKIP_CONFIRMED/NOT EXECUTED、E2E-04 已批准延期、真实升温/光强与跨链路未执行、可选 L3 未解析；'
            'EV-038/EV-037 及更早保留为历史')
new_proto = ('evidence/programmed_prototype.md | active | 下载身份记录（EV-039 置顶：HEAD 9ce380c / 实现提交 82a7e74、'
             '独立重建 axf md5 b6a15fed… 与发布包成员逐字节一致、mdk_flash Verify OK、上电五行与 6 个 q=1 采样行（k=9/12/15/18 连续）；'
             'EV-038/EV-037 及更早保留为历史；仅证明下载成功，不代表功能合格）')

replaced = 0
for i, l in enumerate(lines):
    if l.startswith('evidence/test.md | active |'):
        lines[i] = new_test
        replaced += 1
    elif l.startswith('evidence/programmed_prototype.md | active |'):
        lines[i] = new_proto
        replaced += 1
if replaced != 2:
    raise SystemExit('expected 2 aggregate replacements, got %d' % replaced)

io.open(p, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines))
print('manifest updated; total lines', len(lines))
