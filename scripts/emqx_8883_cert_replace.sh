#!/usr/bin/env bash
# EMQX 8883 服务器证书替换执行器（E2E-JOINT-P0 平台段修复）
#
# 目的：把 EMQX 8883 listener 的叶子证书替换为 SAN 含客户端目标地址（默认 IP:8.140.23.253）、
#       且由 App 信任锚 jd-ca.crt（CN=Tiktok Root CA）签发的证书，并用同一判据复核与自动回退。
#
# 边界（不可通过参数放宽）：
#   * 只改 EMQX 8883 listener 的 certfile/keyfile 及其重载；不改认证库、不改其他 listener、
#     不改 Flask 路由、不连数据库、不新增任何诊断/免校验端口。
#   * 私钥与 CA 私钥只在受控工作目录内使用，不打印、不回传、不写入项目仓库。
#   * 不提供“跳过主机名校验”“-k/--insecure 视为通过”“自签替代 CA”等降级路径。
#
# 模式：
#   recon     只读侦察：EMQX/证书/CA 私钥候选/其他端口基线，输出计划与停止条件，不写任何文件。
#   issue     仅在本地工作目录签发新证书并自检（不安装、不重载）。
#   apply     备份 → 签发 → 自检 → 安装 → 重载 → 复核；复核失败自动回退。
#   rollback  从指定备份目录恢复并重载，复核回到“已知失败”基线。
#
# 退出码：0 符合所选模式期望；2 前置条件不满足（停止，不写）；3 变更后复核失败且已回退；
#         4 变更后复核失败且回退失败（需人工介入）。

set -u

PROG="$(basename "$0")"
HOST="8.140.23.253"
PORT="8883"
CA_KEY=""
CA_FILE=""
CERTFILE=""
KEYFILE=""
DAYS="730"
WORKROOT="/root"
BACKUP_DIR=""
RELOAD_CMD=""
LIVE_CHECK=1
MODE="recon"
BASELINE_SERIAL="77ADCFD924BA0F87F845C81934D4A3F346D4BB82"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

say() { printf '%s\n' "$*"; }
die() { say "STOP: $*"; exit 2; }

usage() {
  sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'
  say ""
  say "用法: $PROG --mode recon|issue|apply|rollback [选项]"
  say "  --ca-key PATH        签发 CA 私钥（Tiktok Root CA）；recon 可省略以只做候选探测"
  say "  --ca FILE            App 信任锚 jd-ca.crt（默认仓库内 assets/jd-ca.crt）"
  say "  --certfile PATH      EMQX 8883 certfile（apply/rollback 必填；recon 可自动探测）"
  say "  --keyfile PATH       EMQX 8883 keyfile（同上）"
  say "  --workdir DIR        工作目录（默认 $WORKROOT/emqx-8883-work-<ts>）"
  say "  --backup-dir DIR     rollback 使用的备份目录（apply 会打印其路径）"
  say "  --reload-cmd 'CMD'   重载命令（默认 'emqx ctl conf reload'；现场确认后覆盖）"
  say "  --no-live-check      apply 后跳过 8883 TLS 复核（不推荐；仅部署环境无法自连时使用）"
  say "  --host / --port      客户端目标地址与端口（默认 $HOST / $PORT）"
}

while [ $# -gt 0 ]; do
  case "$1" in
    --mode) MODE="$2"; shift 2 ;;
    --host) HOST="$2"; shift 2 ;;
    --port) PORT="$2"; shift 2 ;;
    --ca) CA_FILE="$2"; shift 2 ;;
    --ca-key) CA_KEY="$2"; shift 2 ;;
    --certfile) CERTFILE="$2"; shift 2 ;;
    --keyfile) KEYFILE="$2"; shift 2 ;;
    --days) DAYS="$2"; shift 2 ;;
    --workdir) WORKROOT="$2"; shift 2 ;;
    --backup-dir) BACKUP_DIR="$2"; shift 2 ;;
    --reload-cmd) RELOAD_CMD="$2"; shift 2 ;;
    --baseline-serial) BASELINE_SERIAL="$2"; shift 2 ;;
    --no-live-check) LIVE_CHECK=0; shift ;;
    -h|--help) usage; exit 0 ;;
    *) die "未知参数 $1" ;;
  esac
done

[ -n "$CA_FILE" ] || CA_FILE="$REPO_ROOT/dengbei_care/app/src/main/assets/jd-ca.crt"
[ -n "$RELOAD_CMD" ] || RELOAD_CMD="emqx ctl conf reload"
TS="$(date -u +%Y%m%dT%H%M%SZ)"
say "# $PROG mode=$MODE time_utc=$TS host=$HOST port=$PORT"
say "# ca=$CA_FILE"

# ---------- 前置校验 ----------
[ -r "$CA_FILE" ] || die "App 信任锚不可读：$CA_FILE"
verify_ca_fingerprint() {
  fp="$(openssl x509 -in "$CA_FILE" -noout -fingerprint -sha256 2>/dev/null | sed 's/^.*=//')"
  say "FIELD ca_sha256=$fp"
  case "$fp" in
    75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B) ;;
    *) say "WARN: 信任锚指纹与已知基线不同，若确为产品变更需重新评估；本脚本继续但记录该差异" ;;
  esac
}
verify_ca_fingerprint

have_openssl() { command -v openssl >/dev/null 2>&1 || die "缺少 openssl"; }
have_openssl

# ---------- recon ----------
probe_ca_key() {
  # 只做“候选是否存在且与 jd-ca.crt 匹配”的判定，不打印任何私钥内容
  cand=""
  for p in /root/certs/tiktok_root_ca.key /root/tiktok_root_ca.key /root/jd-ca.key \
           /root/certs/jd-ca.key /root/ssl/jd-ca.key /root/cert/jd-ca.key \
           /etc/emqx/certs/jd-ca.key /etc/emqx/certs/ca.key /root/emqx/certs/jd-ca.key; do
    [ -r "$p" ] && cand="$cand $p"
  done
  say "FIELD ca_key_candidates=${cand:-none}"
  [ -n "$CA_KEY" ] && { [ -r "$CA_KEY" ] && say "RESULT ca_key_readable=YES" || say "RESULT ca_key_readable=NO"; }
}

emqx_recon() {
  say "## emqx service"
  if command -v systemctl >/dev/null 2>&1; then
    timeout 20 systemctl status emqx --no-pager 2>&1 | sed -n '1,8p' | sed 's/^/# /'
  fi
  if command -v emqx >/dev/null 2>&1; then
    timeout 20 emqx ctl status 2>&1 | sed -n '1,8p' | sed 's/^/# /'
    say "## listeners"
    timeout 25 emqx ctl listeners 2>&1 | sed -n '1,60p' | sed 's/^/# /'
    say "## listeners.ssl.default"
    timeout 20 emqx ctl conf show listeners.ssl.default 2>&1 | sed -n '1,40p' | sed 's/^/# /'
  else
    say "RESULT emqx_cli=ABSENT (需现场提供 listener 配置文件路径)"
  fi
  if [ -n "$CERTFILE" ] && [ -r "$CERTFILE" ]; then
    say "## deployed leaf"
    timeout 20 openssl x509 -in "$CERTFILE" -noout -subject -issuer -serial -dates \
      -fingerprint -sha256 -ext subjectAltName 2>&1 | sed 's/^/# /'
    say "FIELD deployed_serial=$(openssl x509 -in "$CERTFILE" -noout -serial 2>/dev/null | sed 's/^serial=//')"
    timeout 20 openssl verify -CAfile "$CA_FILE" -verify_ip "$HOST" "$CERTFILE" >/dev/null 2>&1 \
      && say "RESULT deployed_ip_san_match=OK" || say "RESULT deployed_ip_san_match=FAIL"
  else
    say "RESULT certfile_readable=NO (需 --certfile；或由 emqx ctl 输出人工确认路径)"
  fi
  probe_ca_key
  say "## other listeners / flask baseline (connectivity only)"
  for p in 1883 8083 8084 18083 5000; do
    if timeout 5 bash -c "exec 3<>/dev/tcp/127.0.0.1/$p" 2>/dev/null; then
      say "FIELD port_$p=OPEN"; else say "FIELD port_$p=CLOSED"; fi
  done
}

# ---------- 签发 ----------
CURRENT_CERT_CNF() {
  cat >"$1" <<'EOF'
[req]
distinguished_name = dn
prompt             = no
req_extensions     = v3_req
[dn]
C  = US
ST = State
L  = City
O  = Organization
OU = Organizational Unit
CN = HOSTPLACEHOLDER
[v3_req]
basicConstraints = critical,CA:FALSE
keyUsage         = critical,digitalSignature,keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName   = @alt
[alt]
IP.1 = HOSTPLACEHOLDER
IP.2 = 117.72.84.210
[ca_ext]
basicConstraints = critical,CA:TRUE
keyUsage         = critical,keyCertSign,cRLSign
subjectKeyIdentifier = hash
EOF
}

issue_cert() {
  [ -n "$CA_KEY" ] || die "issue/apply 需要 --ca-key（Tiktok Root CA 私钥）；不得自签替代"
  [ -r "$CA_KEY" ] || die "CA 私钥不可读：$CA_KEY"
  # CA 私钥必须与 App 信任锚匹配，否则新证书不会被 App 信任
  ca_pub="$(openssl x509 -in "$CA_FILE" -noout -pubkey 2>/dev/null | openssl sha256 2>/dev/null | sed 's/^.*= //')"
  key_pub="$(openssl pkey -in "$CA_KEY" -pubout 2>/dev/null | openssl sha256 2>/dev/null | sed 's/^.*= //')"
  say "FIELD ca_pub_sha256=$ca_pub"
  say "FIELD cakey_pub_sha256=$key_pub"
  [ -n "$ca_pub" ] && [ "$ca_pub" = "$key_pub" ] || die "CA 私钥与 jd-ca.crt 不匹配：不得用错误 CA 签发"

  umask 077
  WORK="$(mktemp -d "$WORKROOT/emqx-8883-work-XXXXXX" 2>/dev/null || mktemp -d)"
  WORKDIR="$WORK"
  say "FIELD workdir=$WORKDIR"
  CURRENT_CERT_CNF "$WORKDIR/leaf.cnf"
  sed -i "s/HOSTPLACEHOLDER/$HOST/g" "$WORKDIR/leaf.cnf"

  openssl req -new -newkey rsa:2048 -nodes -keyout "$WORKDIR/leaf.key" \
    -out "$WORKDIR/leaf.csr" -config "$WORKDIR/leaf.cnf" >"$WORKDIR/req.log" 2>&1 \
    || { sed 's/^/# /' "$WORKDIR/req.log"; die "CSR 生成失败"; }

  openssl x509 -req -in "$WORKDIR/leaf.csr" -CA "$CA_FILE" -CAkey "$CA_KEY" -CAcreateserial \
    -days "$DAYS" -sha256 -extfile "$WORKDIR/leaf.cnf" -extensions v3_req \
    -out "$WORKDIR/leaf.pem" >"$WORKDIR/sign.log" 2>&1 \
    || { sed 's/^/# /' "$WORKDIR/sign.log"; die "签发失败"; }

  say "## new leaf"
  openssl x509 -in "$WORKDIR/leaf.pem" -noout -subject -issuer -serial -dates \
    -fingerprint -sha256 -ext subjectAltName,extendedKeyUsage 2>&1 | sed 's/^/# /'
  NEW_SERIAL="$(openssl x509 -in "$WORKDIR/leaf.pem" -noout -serial | sed 's/^serial=//')"
  say "FIELD new_serial=$NEW_SERIAL"

  # 自检：链 + IP SAN（权威判据）+ 负例
  openssl verify -CAfile "$CA_FILE" "$WORKDIR/leaf.pem" >"$WORKDIR/chain.txt" 2>&1 \
    && say "RESULT issue_chain_verify=OK" || { sed 's/^/# /' "$WORKDIR/chain.txt"; say "RESULT issue_chain_verify=FAIL"; }
  openssl verify -CAfile "$CA_FILE" -verify_ip "$HOST" "$WORKDIR/leaf.pem" >"$WORKDIR/ip.txt" 2>&1 \
    && say "RESULT issue_ip_san_match=OK" || { sed 's/^/# /' "$WORKDIR/ip.txt"; say "RESULT issue_ip_san_match=FAIL"; }
  openssl verify -CAfile "$CA_FILE" -verify_ip "8.140.23.254" "$WORKDIR/leaf.pem" >/dev/null 2>&1 \
    && say "RESULT issue_negative_control=FAIL (unexpected pass)" || say "RESULT issue_negative_control=OK"
  cert_pub="$(openssl x509 -in "$WORKDIR/leaf.pem" -noout -pubkey | openssl sha256 | sed 's/^.*= //')"
  key_pub2="$(openssl pkey -in "$WORKDIR/leaf.key" -pubout | openssl sha256 | sed 's/^.*= //')"
  [ "$cert_pub" = "$key_pub2" ] && say "RESULT issue_key_matches_cert=OK" || { say "RESULT issue_key_matches_cert=FAIL"; die "新私钥与证书不匹配"; }

  if [ "$MODE" = "apply" ]; then
    # 安装前硬门禁：任一不满足即停止，不触碰线上文件
    openssl verify -CAfile "$CA_FILE" -verify_ip "$HOST" "$WORKDIR/leaf.pem" >/dev/null 2>&1 \
      || die "新证书未通过 IP SAN 自检，禁止部署"
    openssl verify -CAfile "$CA_FILE" "$WORKDIR/leaf.pem" >/dev/null 2>&1 \
      || die "新证书未通过信任锚链校验，禁止部署"
    [ "$NEW_SERIAL" = "$BASELINE_SERIAL" ] && die "新 serial 与基线相同，签发未生效"
  fi
}

# ---------- 部署 / 重载 / 复核 ----------
reload_listener() {
  say "# reload: $RELOAD_CMD"
  bash -c "$RELOAD_CMD" >/tmp/reload_$$.txt 2>&1
  rc=$?
  sed -n '1,20p' /tmp/reload_$$.txt | sed 's/^/# /'
  rm -f /tmp/reload_$$.txt
  say "FIELD reload_rc=$rc"
  sleep 3
  if command -v emqx >/dev/null 2>&1; then
    timeout 25 emqx ctl listeners 2>&1 | sed -n '1,40p' | sed 's/^/# /'
  fi
  return 0
}

live_verify() { # $1=expected  new|baseline
  local exp="$1" rc=0
  if [ "$LIVE_CHECK" -eq 1 ]; then
    if command -v timeout >/dev/null 2>&1; then
      timeout 20 openssl s_client -connect "127.0.0.1:$PORT" -servername "$HOST" \
        -verify_ip "$HOST" -CAfile "$CA_FILE" </dev/null > /tmp/live_$$.txt 2>&1
    else
      openssl s_client -connect "127.0.0.1:$PORT" -servername "$HOST" \
        -verify_ip "$HOST" -CAfile "$CA_FILE" </dev/null > /tmp/live_$$.txt 2>&1
    fi
    grep -E "Verify return code|subject=|issuer=" /tmp/live_$$.txt | sed -n '1,6p' | sed 's/^/# /'
    grep -q "Verify return code: 0 (ok)" /tmp/live_$$.txt || rc=1
    rm -f /tmp/live_$$.txt
  fi
  for p in 8883 1883 8083 8084 18083 5000; do
    if timeout 5 bash -c "exec 3<>/dev/tcp/127.0.0.1/$p" 2>/dev/null; then
      say "FIELD port_$p=OPEN"; else say "FIELD port_$p=CLOSED"; fi
  done
  local s
  s="$(openssl x509 -in "$CERTFILE" -noout -serial 2>/dev/null | sed 's/^serial=//')"
  say "FIELD installed_serial=$s"
  if [ "$exp" = "new" ]; then
    [ "$rc" -eq 0 ] || { say "RESULT live_verify=FAIL"; return 1; }
    openssl verify -CAfile "$CA_FILE" -verify_ip "$HOST" "$CERTFILE" >/dev/null 2>&1 \
      || { say "RESULT live_verify=FAIL (ip san)"; return 1; }
    [ "$s" != "$BASELINE_SERIAL" ] || { say "RESULT live_verify=FAIL (serial unchanged)"; return 1; }
    say "RESULT live_verify=OK"; return 0
  else
    [ "$s" = "$BASELINE_SERIAL" ] && say "RESULT baseline_restored=YES" || say "RESULT baseline_restored=NO"
    say "RESULT live_verify=RECORDED (baseline remains a known failure)"
    return 0
  fi
}

apply_change() {
  [ -n "$CERTFILE" ] && [ -n "$KEYFILE" ] || die "apply 需要 --certfile 与 --keyfile（取自 recon 的 emqx ctl 输出）"
  [ -r "$CERTFILE" ] || die "certfile 不可读：$CERTFILE"
  [ -r "$KEYFILE" ] || die "keyfile 不可读：$KEYFILE"

  issue_cert

  BK="$WORKDIR/backup"; mkdir -p "$BK" && chmod 700 "$BK" 2>/dev/null || true
  cp -a "$CERTFILE" "$BK/leaf.crt.bak"; cp -a "$KEYFILE" "$BK/leaf.key.bak"
  ( cd "$BK" && sha256sum leaf.crt.bak leaf.key.bak > SHA256SUMS.txt ) 2>/dev/null || true
  say "FIELD backup_dir=$BK"
  ( cd "$BK" && cat SHA256SUMS.txt 2>/dev/null | sed 's/^/# /' )
  cp -a "$CERTFILE" "$CERTFILE.pre-$TS" 2>/dev/null || true

  # 保留原文件属主/属组，避免 EMQX 工作用户无法读取私钥导致 listener 起不来
  OWNER_C="$(stat -c '%u:%g' "$CERTFILE" 2>/dev/null || true)"
  OWNER_K="$(stat -c '%u:%g' "$KEYFILE" 2>/dev/null || true)"
  say "FIELD owner_cert=${OWNER_C:-unknown} owner_key=${OWNER_K:-unknown}"

  install -m 644 "$WORKDIR/leaf.pem" "$CERTFILE" || die "安装证书失败（未重载，现有配置未变）"
  install -m 600 "$WORKDIR/leaf.key" "$KEYFILE" || die "安装私钥失败（未重载；请立即用备份恢复）"
  [ -n "$OWNER_C" ] && chown "$OWNER_C" "$CERTFILE" 2>/dev/null || true
  [ -n "$OWNER_K" ] && chown "$OWNER_K" "$KEYFILE" 2>/dev/null || true

  reload_listener
  if live_verify new; then
    say "RESULT overall=APPLIED"
    say "# 下一步（不由本脚本代判）：交回软件测试在同一用例/同一 App/同一手机复验 App→EMQX TLS/CONNACK"
    return 0
  fi

  say "## 复核失败，执行回退"
  cp -a "$BK/leaf.crt.bak" "$CERTFILE"; cp -a "$BK/leaf.key.bak" "$KEYFILE"
  [ -n "$OWNER_C" ] && chown "$OWNER_C" "$CERTFILE" 2>/dev/null || true
  [ -n "$OWNER_K" ] && chown "$OWNER_K" "$KEYFILE" 2>/dev/null || true
  reload_listener
  if live_verify baseline; then
    say "RESULT overall=ROLLED_BACK (变更未生效，回到已知失败基线)"
    return 3
  fi
  say "RESULT overall=ROLLBACK_FAILED (需人工介入)"
  return 4
}

rollback_only() {
  [ -n "$BACKUP_DIR" ] || die "rollback 需要 --backup-dir"
  [ -r "$BACKUP_DIR/leaf.crt.bak" ] || die "备份缺失：$BACKUP_DIR/leaf.crt.bak"
  [ -n "$CERTFILE" ] && [ -n "$KEYFILE" ] || die "rollback 需要 --certfile 与 --keyfile"
  cp -a "$BACKUP_DIR/leaf.crt.bak" "$CERTFILE" || die "恢复证书失败"
  cp -a "$BACKUP_DIR/leaf.key.bak" "$KEYFILE" || die "恢复私钥失败"
  # cp -a 已保留备份的属主/权限；备份取自原文件，故回退后属性一致
  reload_listener
  live_verify baseline
  say "RESULT overall=ROLLED_BACK"
}

case "$MODE" in
  recon) emqx_recon; say "RESULT overall=RECORDED" ;;
  issue) issue_cert; say "RESULT overall=ISSUED (未安装、未重载)" ;;
  apply) apply_change; exit $? ;;
  rollback) rollback_only ;;
  *) die "未知 mode=$MODE" ;;
esac
