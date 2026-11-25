#!/usr/bin/env bash
# 在 EMQX 服务器上安装 8883 叶子证书：门禁 → 备份 → 安装 → 逐级重载 → 复核 → 失败自动回退
#
# 用法（在服务器上，root）：
#   bash apply_remote.sh \
#     --workdir  /root/rd_ops_<date> \
#     --certfile /var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA \
#     --keyfile  /var/lib/emqx/certs/listeners/ssl/default/key-6E6F6ED56285A948 \
#     --ca       /root/rd_ops_<date>/jd-ca.crt \
#     --host-ip  8.140.23.253
#
# workdir 内需包含：leaf.pem（新证书）、leaf.key（新私钥，600）、jd-ca.crt（App 信任锚）。
#
# 实测要点（2026-11-25）：本部署上 `emqx ctl listeners restart` 与 `emqx ctl conf reload`
# 都不会让 listener 重新读取磁盘证书，必须 `systemctl restart emqx` 才生效；因此本脚本
# 按 conf reload → service restart 的顺序尝试，并在复核失败时自动回退。
#
# 退出码：0 已应用且复核通过；2 门禁不通过（未触碰线上文件）；3 复核失败且已回退；4 安装失败。

set -u

WORKDIR=""
CERTFILE=""
KEYFILE=""
CA=""
HOST_IP="8.140.23.253"
BASE_SERIAL="77ADCFD924BA0F87F845C81934D4A3F346D4BB82"
CHECK_PORTS="1883 8883 8083 8084 18083 5000 3306"

while [ $# -gt 0 ]; do
  case "$1" in
    --workdir) WORKDIR="$2"; shift 2 ;;
    --certfile) CERTFILE="$2"; shift 2 ;;
    --keyfile) KEYFILE="$2"; shift 2 ;;
    --ca) CA="$2"; shift 2 ;;
    --host-ip) HOST_IP="$2"; shift 2 ;;
    --baseline-serial) BASE_SERIAL="$2"; shift 2 ;;
    -h|--help) sed -n '2,22p' "$0"; exit 0 ;;
    *) echo "unknown arg $1" >&2; exit 64 ;;
  esac
done

[ -n "$WORKDIR" ] && [ -n "$CERTFILE" ] && [ -n "$KEYFILE" ] && [ -n "$CA" ] \
  || { echo "ERROR: 需要 --workdir/--certfile/--keyfile/--ca" >&2; exit 64; }
NEW="$WORKDIR/leaf.pem"; NEWKEY="$WORKDIR/leaf.key"
for f in "$NEW" "$NEWKEY" "$CA" "$CERTFILE" "$KEYFILE"; do
  [ -r "$f" ] || { echo "ERROR: 不可读 $f" >&2; exit 66; }
done

TS=$(date -u +%Y%m%dT%H%M%SZ)
BK="/root/emqx-8883-cert-backup-$TS"
LOG="$WORKDIR/apply.log"
exec > >(tee "$LOG") 2>&1

NEW_SERIAL=$(openssl x509 -in "$NEW" -noout -serial | sed 's/^serial=//')
echo "# apply start $TS"
echo "FIELD host_ip=$HOST_IP"
echo "FIELD new_serial_expected=$NEW_SERIAL"
echo "FIELD certfile=$CERTFILE"
echo "FIELD keyfile=$KEYFILE"

# ---------- 门禁 ----------
openssl verify -CAfile "$CA" "$NEW" >/dev/null 2>&1 || { echo "STOP gate_chain=FAIL"; exit 2; }
openssl verify -CAfile "$CA" -verify_ip "$HOST_IP" "$NEW" >/dev/null 2>&1 || { echo "STOP gate_ip_san=FAIL"; exit 2; }
openssl x509 -in "$NEW" -noout -ext subjectAltName | grep -q "$HOST_IP" || { echo "STOP gate_san_text=FAIL"; exit 2; }
[ "$(openssl x509 -in "$NEW" -noout -pubkey | openssl sha256)" = \
  "$(openssl pkey -in "$NEWKEY" -pubout | openssl sha256)" ] || { echo "STOP gate_keypair=FAIL"; exit 2; }
echo "RESULT gate_self_check=OK"

# ---------- 备份 ----------
umask 077
mkdir -p "$BK"
cp -a "$CERTFILE" "$BK/leaf.crt.bak"
cp -a "$KEYFILE" "$BK/leaf.key.bak"
( cd "$BK" && sha256sum leaf.crt.bak leaf.key.bak > SHA256SUMS.txt )
chmod 600 "$BK/leaf.key.bak"
echo "FIELD backup_dir=$BK"
cat "$BK/SHA256SUMS.txt"

# ---------- 安装 ----------
install -m 644 -o emqx -g emqx "$NEW" "$CERTFILE" || { echo "RESULT install_cert=FAIL"; exit 4; }
install -m 600 -o emqx -g emqx "$NEWKEY" "$KEYFILE" || {
  echo "RESULT install_key=FAIL"
  cp -a "$BK/leaf.crt.bak" "$CERTFILE"; cp -a "$BK/leaf.key.bak" "$KEYFILE"; exit 4; }
echo "FIELD installed_cert_sha256=$(sha256sum "$CERTFILE" | cut -d' ' -f1)"
echo "FIELD installed_key_mode=$(stat -c '%a' "$KEYFILE")"
echo "RESULT install=OK"

served_serial() {
  timeout 20 openssl s_client -connect 127.0.0.1:8883 -verify_ip "$HOST_IP" -CAfile "$CA" </dev/null 2>/dev/null \
    | openssl x509 -noout -serial 2>/dev/null | sed 's/^serial=//'
}

check_new() {
  local rc=0 s
  timeout 20 openssl s_client -connect 127.0.0.1:8883 -verify_ip "$HOST_IP" -CAfile "$CA" </dev/null >"$WORKDIR/live.txt" 2>&1
  grep -E "Verify return code|subject=|issuer=" "$WORKDIR/live.txt" | head -4
  grep -q "Verify return code: 0 (ok)" "$WORKDIR/live.txt" || rc=1
  s=$(served_serial); echo "FIELD served_serial=$s"
  [ "$s" = "$NEW_SERIAL" ] || rc=1
  emqx ctl listeners 2>/dev/null | grep -A6 "ssl:default" | grep -q "running         : true" || rc=1
  return $rc
}

APPLIED=0

echo "## step A: emqx ctl conf reload"
timeout 60 emqx ctl conf reload; echo "FIELD conf_reload_rc=$?"
sleep 4
if check_new; then echo "RESULT reload_mode=conf_reload"; APPLIED=1; fi

if [ "$APPLIED" -eq 0 ]; then
  echo "## step B: systemctl restart emqx（conf reload 未使 listener 重读证书）"
  echo "FIELD uptime_before=$(emqx ctl broker 2>/dev/null | grep '^uptime')"
  timeout 90 systemctl restart emqx; echo "FIELD restart_rc=$?"
  for i in $(seq 1 30); do
    sleep 1
    if emqx ctl status >/dev/null 2>&1; then echo "FIELD emqx_up_after_s=$i"; break; fi
  done
  sleep 2
  if check_new; then echo "RESULT reload_mode=service_restart"; APPLIED=1; fi
fi

if [ "$APPLIED" -eq 1 ]; then
  echo "RESULT live_verify=OK"
  echo "## post-change regression"
  emqx ctl listeners | sed -n '1,40p'
  echo "FIELD uptime_after=$(emqx ctl broker 2>/dev/null | grep '^uptime')"
  for p in $CHECK_PORTS; do
    if timeout 5 bash -c "exec 3<>/dev/tcp/127.0.0.1/$p" 2>/dev/null; then echo "PORT $p OPEN"; else echo "PORT $p CLOSED"; fi
  done
  timeout 10 curl -s -o /dev/null -w "flask_login_get_http=%{http_code}\n" http://127.0.0.1:5000/login
  timeout 10 curl -s -o /dev/null -w "dashboard_http=%{http_code}\n" http://127.0.0.1:18083/status
  echo "RESULT overall=APPLIED"
  exit 0
fi

echo "## both reload modes failed -> rollback"
cp -a "$BK/leaf.crt.bak" "$CERTFILE"; cp -a "$BK/leaf.key.bak" "$KEYFILE"
chown emqx:emqx "$CERTFILE" "$KEYFILE" 2>/dev/null || true
timeout 90 systemctl restart emqx; sleep 8
echo "FIELD restored_served_serial=$(served_serial)"
emqx ctl listeners | grep -A6 "ssl:default" | grep "running"
echo "RESULT overall=ROLLED_BACK"
exit 3
