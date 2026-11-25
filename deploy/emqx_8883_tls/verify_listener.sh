#!/usr/bin/env bash
# 服务器侧只读复核：8883 证书身份 + listener + 端口/服务回归
#
# 用法：
#   bash verify_listener.sh [--certfile ...] [--ca ...] [--host-ip 8.140.23.253]
#                            [--expect-serial 77ADCFD9…B83 | --expect-baseline]
#
# 只读取证书与握手，不修改任何配置。

set -u
CERTFILE=/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA
CA=/root/rd_ops_20261125/jd-ca.crt
HOST_IP=8.140.23.253
EXPECT_SERIAL=""
BASE_SERIAL=77ADCFD924BA0F87F845C81934D4A3F346D4BB82

while [ $# -gt 0 ]; do
  case "$1" in
    --certfile) CERTFILE="$2"; shift 2 ;;
    --ca) CA="$2"; shift 2 ;;
    --host-ip) HOST_IP="$2"; shift 2 ;;
    --expect-serial) EXPECT_SERIAL="$2"; shift 2 ;;
    --expect-baseline) EXPECT_SERIAL="$BASE_SERIAL"; shift ;;
    *) echo "unknown arg $1" >&2; exit 64 ;;
  esac
done

echo "# EMQX 8883 verify time_utc=$(date -u +%Y%m%dT%H%M%SZ)"
echo "## listener"
emqx ctl listeners 2>&1 | sed -n '1,40p'
echo "## deployed file"
ls -l "$CERTFILE" "${CERTFILE%/*}"/* 2>/dev/null | head -6
openssl x509 -in "$CERTFILE" -noout -subject -issuer -serial -dates -fingerprint -sha256 -ext subjectAltName
S=$(openssl x509 -in "$CERTFILE" -noout -serial | sed 's/^serial=//')
echo "FIELD file_serial=$S"
echo "## ip san"
openssl verify -CAfile "$CA" -verify_ip "$HOST_IP" "$CERTFILE" >/dev/null 2>&1 \
  && echo "RESULT file_ip_san_match=OK" || echo "RESULT file_ip_san_match=FAIL"
echo "## tls handshake on 127.0.0.1:8883"
timeout 20 openssl s_client -connect 127.0.0.1:8883 -verify_ip "$HOST_IP" -CAfile "$CA" </dev/null 2>&1 \
  | grep -E "Verify return code|subject=|issuer=" | head -4
echo "## ports"
for p in 1883 8883 8083 8084 18083 5000 3306; do
  if timeout 5 bash -c "exec 3<>/dev/tcp/127.0.0.1/$p" 2>/dev/null; then echo "PORT $p OPEN"; else echo "PORT $p CLOSED"; fi
done
timeout 10 curl -s -o /dev/null -w "flask_login_get_http=%{http_code}\n" http://127.0.0.1:5000/login
timeout 10 curl -s -o /dev/null -w "dashboard_http=%{http_code}\n" http://127.0.0.1:18083/status

if [ -n "$EXPECT_SERIAL" ]; then
  if [ "$S" = "$EXPECT_SERIAL" ]; then echo "RESULT serial_match=YES ($S)"; else echo "RESULT serial_match=NO (file=$S expected=$EXPECT_SERIAL)"; fi
fi
