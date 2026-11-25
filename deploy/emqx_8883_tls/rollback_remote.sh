#!/usr/bin/env bash
# 回退 EMQX 8883 叶子证书到备份（变更前基线）
#
# 用法：
#   bash rollback_remote.sh --backup-dir /root/emqx-8883-cert-backup-<ts> \
#     --certfile /var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA \
#     --keyfile  /var/lib/emqx/certs/listeners/ssl/default/key-6E6F6ED56285A948 \
#     --ca       /root/rd_ops_<date>/jd-ca.crt
#
# 回退后仍为“已知失败”状态（叶子证书 SAN 只有旧 IP），不得计为通过。

set -u
BACKUP_DIR=""
CERTFILE=""
KEYFILE=""
CA=""
BASE_SERIAL=77ADCFD924BA0F87F845C81934D4A3F346D4BB82

while [ $# -gt 0 ]; do
  case "$1" in
    --backup-dir) BACKUP_DIR="$2"; shift 2 ;;
    --certfile) CERTFILE="$2"; shift 2 ;;
    --keyfile) KEYFILE="$2"; shift 2 ;;
    --ca) CA="$2"; shift 2 ;;
    *) echo "unknown arg $1" >&2; exit 64 ;;
  esac
done

[ -r "$BACKUP_DIR/leaf.crt.bak" ] || { echo "ERROR: 备份缺失 $BACKUP_DIR/leaf.crt.bak" >&2; exit 66; }
[ -r "$BACKUP_DIR/leaf.key.bak" ] || { echo "ERROR: 备份缺失 $BACKUP_DIR/leaf.key.bak" >&2; exit 66; }

echo "# rollback start $(date -u +%Y%m%dT%H%M%SZ)"
install -m 644 -o emqx -g emqx "$BACKUP_DIR/leaf.crt.bak" "$CERTFILE" || exit 4
install -m 600 -o emqx -g emqx "$BACKUP_DIR/leaf.key.bak" "$KEYFILE" || exit 4
chown emqx:emqx "$CERTFILE" "$KEYFILE" 2>/dev/null || true

timeout 90 systemctl restart emqx; echo "FIELD restart_rc=$?"
for i in $(seq 1 30); do sleep 1; emqx ctl status >/dev/null 2>&1 && { echo "FIELD emqx_up_after_s=$i"; break; }; done
sleep 2

S=$(timeout 20 openssl s_client -connect 127.0.0.1:8883 -CAfile "$CA" </dev/null 2>/dev/null \
    | openssl x509 -noout -serial 2>/dev/null | sed 's/^serial=//')
echo "FIELD served_serial=$S"
if [ "$S" = "$BASE_SERIAL" ]; then echo "RESULT baseline_restored=YES"; else echo "RESULT baseline_restored=NO"; fi
emqx ctl listeners 2>/dev/null | grep -A6 "ssl:default" | grep -E "running|listen_on"
echo "RESULT overall=ROLLED_BACK（仍为已知失败状态）"
