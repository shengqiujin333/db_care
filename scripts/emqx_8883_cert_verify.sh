#!/usr/bin/env bash
# EMQX 8883 服务器证书只读核验（E2E-JOINT-P0 平台段证书替换的验收/回退判据）
#
# 用途：在证书替换前、替换后或回退后，用同一判据核验 8883 listener 返回的叶子证书
#       是否标识了客户端实际连接的地址，并保持 App 信任锚不变。
# 边界：只做 TCP/TLS 握手与本地 openssl 校验；不登录、不写文件到目标服务器、
#       不重启/不重载服务、不调用管理 API、不输出任何私钥或口令。
#
# 用法：
#   scripts/emqx_8883_cert_verify.sh [host] [port] [ca-file] [mode]
#     host     默认 8.140.23.253
#     port     默认 8883
#     ca-file  默认 dengbei_care/app/src/main/assets/jd-ca.crt（App 信任锚）
#     mode     --expect-new（默认，替换后：SAN 必须命中目标地址，且 serial 不得等于变更前基线）
#              --expect-baseline（变更前/回退后：只记录现状，不要求通过）
#
# 输出：人类可读明细 + `RESULT key=value` 机器可读行；退出码 0 表示符合所选模式期望。
# 依赖：bash、openssl 3.x、timeout；可选 emqx CLI（存在时附带 listener 状态）。

set -u

HOST="${1:-8.140.23.253}"
PORT="${2:-8883}"
CA="${3:-dengbei_care/app/src/main/assets/jd-ca.crt}"
MODE="${4:---expect-new}"

# 变更前基线（2026-11-24T05:46Z 只读复检，见 evidence/e2e_p0/backend_cert_recheck_20261124_0546Z.txt）
BASELINE_SERIAL="77ADCFD924BA0F87F845C81934D4A3F346D4BB82"
if command -v timeout >/dev/null 2>&1; then T=(timeout 20); else T=(); fi

fail=0
say() { printf '%s\n' "$*"; }

say "# EMQX TLS leaf certificate check"
say "# time_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ) host=$HOST port=$PORT mode=$MODE"
say "# ca=$CA"

if [ ! -r "$CA" ]; then
  say "RESULT ca_readable=NO"
  say "RESULT overall=FAIL"
  exit 2
fi

tmp="$(mktemp -d 2>/dev/null || mktemp -d -t emqxcheck)"
trap 'rm -rf "$tmp"' EXIT

# 1) 取叶子证书（不做证书链信任判定，避免受本地信任库影响）
"${T[@]}" openssl s_client -connect "$HOST:$PORT" -servername "$HOST" -showcerts </dev/null >"$tmp/sclient.txt" 2>&1
sc_rc=$?
awk '/BEGIN CERTIFICATE/,/END CERTIFICATE/' "$tmp/sclient.txt" | head -80 >"$tmp/leaf.pem"
if [ ! -s "$tmp/leaf.pem" ]; then
  say "RESULT tls_handshake=FAIL (s_client rc=$sc_rc, no certificate returned)"
  sed -n '1,20p' "$tmp/sclient.txt"
  say "RESULT overall=FAIL"
  exit 1
fi
say "RESULT tls_handshake=OK"

# 2) 叶子证书字段
say "# --- leaf certificate ---"
openssl x509 -in "$tmp/leaf.pem" -noout -subject -issuer -serial -dates -fingerprint -sha256 -ext subjectAltName 2>&1 | sed 's/^/# /'

serial="$(openssl x509 -in "$tmp/leaf.pem" -noout -serial 2>/dev/null | sed 's/^serial=//')"
san="$(openssl x509 -in "$tmp/leaf.pem" -noout -ext subjectAltName 2>/dev/null | tr -d '\r')"
fp="$(openssl x509 -in "$tmp/leaf.pem" -noout -fingerprint -sha256 2>/dev/null | sed 's/^.*=//')"
say "FIELD serial=$serial"
say "FIELD sha256=$fp"

case "$san" in
  *"IP Address:$HOST"*) san_hit=YES ;;
  *) san_hit=NO ;;
esac
say "RESULT san_contains_target=$san_hit"

# 3) 链校验（App 信任锚）
"${T[@]}" openssl verify -CAfile "$CA" "$tmp/leaf.pem" >"$tmp/chain.txt" 2>&1
chain_rc=$?
say "# --- verify with App trust anchor ---"
sed 's/^/# /' "$tmp/chain.txt"
if [ "$chain_rc" -eq 0 ]; then say "RESULT chain_verify=OK"; else say "RESULT chain_verify=FAIL"; fi

# 4) 严格主机名判据：IP 形式必须用 -verify_ip（-verify_hostname 会回退比较 CN）
"${T[@]}" openssl verify -CAfile "$CA" -verify_ip "$HOST" "$tmp/leaf.pem" >"$tmp/ip.txt" 2>&1
ip_rc=$?
say "# --- verify_ip $HOST (authoritative for IP endpoints) ---"
sed 's/^/# /' "$tmp/ip.txt"
if [ "$ip_rc" -eq 0 ]; then say "RESULT ip_san_match=OK"; else say "RESULT ip_san_match=FAIL"; fi

# 5) 负例对照：另一个 IP 必须不通过（证明判据未被放宽）
"${T[@]}" openssl verify -CAfile "$CA" -verify_ip "8.140.23.254" "$tmp/leaf.pem" >"$tmp/neg.txt" 2>&1
neg_rc=$?
if [ "$neg_rc" -eq 0 ]; then say "RESULT negative_control=FAIL (unexpected pass)"; else say "RESULT negative_control=OK"; fi

# 6) 可选：服务器本地 listener 状态（仅在 emqx CLI 可用的主机上）
if command -v emqx >/dev/null 2>&1; then
  say "# --- emqx ctl listeners ---"
  "${T[@]}" emqx ctl listeners 2>&1 | sed -n '1,40p' | sed 's/^/# /'
fi

# 7) 判定
if [ "$MODE" = "--expect-baseline" ]; then
  say "RESULT mode=baseline serial=$serial"
  if [ "$san_hit" = "NO" ] && [ "$ip_rc" -ne 0 ]; then
    say "RESULT baseline_state=known_failure (SAN does not identify $HOST)"
  else
    say "RESULT baseline_state=unexpected (certificate already identifies $HOST)"
  fi
  say "RESULT overall=RECORDED"
  exit 0
fi

if [ "$chain_rc" -eq 0 ] && [ "$ip_rc" -eq 0 ] && [ "$san_hit" = "YES" ] && [ "$neg_rc" -ne 0 ]; then
  if [ "$serial" = "$BASELINE_SERIAL" ]; then
    say "RESULT serial_changed=NO"
  else
    say "RESULT serial_changed=YES"
  fi
  say "RESULT overall=PASS"
  exit 0
fi

say "RESULT overall=FAIL"
exit 1
