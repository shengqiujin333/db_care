#!/usr/bin/env bash
# 签发 EMQX 8883 叶子证书（由 App 信任锚 jd-ca.crt 签发）
#
# 用法：
#   ./issue_leaf.sh --ca-key <Tiktok Root CA 私钥路径> \
#                   [--ca dengbei_care/app/src/main/assets/jd-ca.crt] \
#                   [--host-ip 8.140.23.253] [--legacy-ip 117.72.84.210] \
#                   [--days 730] [--out-dir <目录>] [--cnf <模板>]
#
# 硬约束：
#   * CA 私钥必须与 --ca 证书公钥一致，否则拒绝签发（错 CA 签出的证书 App 不信任）。
#   * 签发前核对 CA 指纹为已知基线；不一致则不签发（防止信任锚被更换后静默换签）。
#   * 签发后必须通过：链校验、-verify_ip 命中、负例 IP 不命中、私钥与证书公钥一致。
#   * 不打印私钥内容；输出目录 umask 077；私钥文件权限 600。
#   * 私钥不得复制进项目仓库；签发完成后按运行要求只保留部署副本。

set -euo pipefail

CA="dengbei_care/app/src/main/assets/jd-ca.crt"
CA_KEY=""
HOST_IP="8.140.23.253"
LEGACY_IP="117.72.84.210"
DAYS="730"
OUT_DIR=""
CNF="$(cd "$(dirname "$0")" && pwd)/leaf-openssl.cnf"
KNOWN_CA_FP="75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B"

while [ $# -gt 0 ]; do
  case "$1" in
    --ca) CA="$2"; shift 2 ;;
    --ca-key) CA_KEY="$2"; shift 2 ;;
    --host-ip) HOST_IP="$2"; shift 2 ;;
    --legacy-ip) LEGACY_IP="$2"; shift 2 ;;
    --days) DAYS="$2"; shift 2 ;;
    --out-dir) OUT_DIR="$2"; shift 2 ;;
    --cnf) CNF="$2"; shift 2 ;;
    -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
    *) echo "unknown arg $1" >&2; exit 64 ;;
  esac
done

[ -n "$CA_KEY" ] || { echo "ERROR: 必须用 --ca-key 指定 CA 私钥；不允许自签替代" >&2; exit 64; }
[ -r "$CA" ] || { echo "ERROR: CA 证书不可读: $CA" >&2; exit 66; }
[ -r "$CA_KEY" ] || { echo "ERROR: CA 私钥不可读: $CA_KEY" >&2; exit 66; }
[ -r "$CNF" ] || { echo "ERROR: 模板不可读: $CNF" >&2; exit 66; }

TS=$(date -u +%Y%m%dT%H%M%SZ)
[ -n "$OUT_DIR" ] || OUT_DIR="./emqx-8883-issue-$TS"
umask 077
mkdir -p "$OUT_DIR"

# --- CA 身份与私钥匹配（不输出私钥内容） ---
CA_FP=$(openssl x509 -in "$CA" -noout -fingerprint -sha256 | sed 's/^.*=//')
echo "FIELD ca_fingerprint=$CA_FP"
[ "$CA_FP" = "$KNOWN_CA_FP" ] || { echo "STOP: CA 证书指纹与已知信任锚基线不同，拒绝签发" >&2; exit 2; }
CA_PUB=$(openssl x509 -in "$CA" -noout -pubkey | openssl sha256 | sed 's/^.*= //')
KEY_PUB=$(openssl pkey -in "$CA_KEY" -pubout -passin env:EMQX_CA_KEY_PASSPHRASE 2>/dev/null | openssl sha256 | sed 's/^.*= //')
[ -n "$KEY_PUB" ] || { echo "STOP: 无法读取 CA 私钥（口令请通过环境变量 EMQX_CA_KEY_PASSPHRASE 提供）" >&2; exit 2; }
[ "$CA_PUB" = "$KEY_PUB" ] || { echo "STOP: CA 私钥与 CA 证书不匹配，拒绝签发" >&2; exit 2; }
echo "RESULT ca_key_matches_ca_cert=YES"

# --- 由模板生成带目标 IP 的配置 ---
CNF_RUN="$OUT_DIR/leaf.cnf"
sed -e "s/^CN = .*/CN = $HOST_IP/" \
    -e "s/^IP.1 = .*/IP.1 = $HOST_IP/" \
    -e "s/^IP.2 = .*/IP.2 = $LEGACY_IP/" "$CNF" > "$CNF_RUN"

openssl req -new -newkey rsa:2048 -nodes -keyout "$OUT_DIR/leaf.key" -out "$OUT_DIR/leaf.csr" \
  -config "$CNF_RUN" >"$OUT_DIR/req.log" 2>&1

openssl x509 -req -in "$OUT_DIR/leaf.csr" -CA "$CA" -CAkey "$CA_KEY" \
  -passin env:EMQX_CA_KEY_PASSPHRASE -CAcreateserial \
  -days "$DAYS" -sha256 -extfile "$CNF_RUN" -extensions v3_req \
  -out "$OUT_DIR/leaf.pem" >"$OUT_DIR/sign.log" 2>&1

echo "## issued leaf"
openssl x509 -in "$OUT_DIR/leaf.pem" -noout -subject -issuer -serial -dates \
  -fingerprint -sha256 -ext subjectAltName,extendedKeyUsage

# --- 部署前自检 ---
openssl verify -CAfile "$CA" "$OUT_DIR/leaf.pem" >/dev/null 2>&1 \
  && echo "RESULT issue_chain_verify=OK" || { echo "RESULT issue_chain_verify=FAIL" >&2; exit 3; }
openssl verify -CAfile "$CA" -verify_ip "$HOST_IP" "$OUT_DIR/leaf.pem" >/dev/null 2>&1 \
  && echo "RESULT issue_ip_san_match=OK" || { echo "RESULT issue_ip_san_match=FAIL" >&2; exit 3; }
if openssl verify -CAfile "$CA" -verify_ip "203.0.113.1" "$OUT_DIR/leaf.pem" >/dev/null 2>&1; then
  echo "RESULT issue_negative_control=FAIL (unexpected pass)" >&2; exit 3
else
  echo "RESULT issue_negative_control=OK"
fi
[ "$(openssl x509 -in "$OUT_DIR/leaf.pem" -noout -pubkey | openssl sha256)" = \
  "$(openssl pkey -in "$OUT_DIR/leaf.key" -pubout | openssl sha256)" ] \
  && echo "RESULT issue_keypair_match=OK" || { echo "RESULT issue_keypair_match=FAIL" >&2; exit 3; }

chmod 600 "$OUT_DIR/leaf.key"
echo "FIELD out_dir=$OUT_DIR"
echo "FIELD leaf_private_key_mode=$(stat -c '%a' "$OUT_DIR/leaf.key" 2>/dev/null || echo unknown)"
echo "RESULT overall=ISSUED (未安装、未重载)"
echo "# 私钥仅用于部署：上传后应删除本地副本；不要复制进项目仓库或证据。"
