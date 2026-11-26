"""Bounded CodeGraph validation; only existing project indexes are synchronized."""
import datetime, hashlib, json, pathlib, re, shutil, subprocess, sys

BASE = pathlib.Path(__file__).resolve().parents[1]
OUT = BASE / 'evidence/codegraph_alarm_chain_20261126.json'
CLI = shutil.which('codegraph.cmd')
def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()
def run(args, limit=30):
    start = now()
    p = subprocess.Popen(args, cwd=BASE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    timed_out = False
    try:
        out, err = p.communicate(timeout=limit)
    except subprocess.TimeoutExpired:
        timed_out = True
        subprocess.run(['taskkill', '/PID', str(p.pid), '/T', '/F'], capture_output=True, timeout=10, check=True)
        out, err = p.communicate(timeout=10)
    return dict(command=args, started_utc=start, ended_utc=now(), timeout_seconds=limit,
                exit_code=p.returncode, timed_out=timed_out,
                stdout=out.decode('utf-8', 'replace'), stderr=err.decode('utf-8', 'replace'))
def git(*args):
    return run(['git', *args])

if '--exact-followup' in sys.argv:
    data = json.loads(OUT.read_text(encoding='utf-8'))
    for record in data['roots']:
        for prior in list(record['queries']):
            if prior['current_source_located']:
                continue
            target = prior['target']
            result = run([CLI, 'explore', '-p', record['projectPath'], '--max-files', '1', target])
            raw = result.pop('stdout')
            lines = raw.splitlines()
            numbered = [line for line in lines if re.match(r'^\d+\t', line)]
            result.update(query=target, target=target, target_sha256=prior['target_sha256'],
                          response_sha256=hashlib.sha256(raw.encode('utf-8')).hexdigest(),
                          source_headers=[line for line in lines if line.startswith('**`')],
                          call_context=[line for line in lines if line.startswith('- `')],
                          numbered_source_excerpt=numbered[:45],
                          current_source_located=('**`'+target+'`**' in raw and bool(numbered)),
                          reason='Symbol-only ranking selected a related file; exact-path query validates intended source')
            record['queries'].append(result)
            print(target, result['exit_code'], result['current_source_located'])
    data['ended_utc'] = now()
    OUT.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    sys.exit(0)

specs = [
 ('sensor', 'CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output',
  [('gxht40_measure sensor_decide_report light_is_dark', 3, 'USER/src/fw_core.c'),
   ('measure_sample', 1, 'USER/src/measure.c')]),
 ('gateway', 'CH592EVT/EVT/EXAM/BLE/beiwov2',
  [('decode_frame10', 1, 'APP/feistel_al.c'), ('app_um2006A_run ble_send_bf', 1, 'APP/app_um2006A.c')]),
 ('android', 'dengbei_care',
  [('app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt', 1, 'app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt'), ('app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt', 1, 'app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt'), ('app/src/main/java/com/jinyuni/dengbei_care/ui/home/HomeFragment.kt', 1, 'app/src/main/java/com/jinyuni/dengbei_care/ui/home/HomeFragment.kt'), ('app/src/main/java/com/jinyuni/dengbei_care/BaseActivity.kt', 1, 'app/src/main/java/com/jinyuni/dengbei_care/BaseActivity.kt'), ('app/src/main/java/com/jinyuni/dengbei_care/RingtonePlayer.kt', 1, 'app/src/main/java/com/jinyuni/dengbei_care/RingtonePlayer.kt'), ('app/src/main/java/com/jinyuni/dengbei_care/VibrationPlayer.kt', 1, 'app/src/main/java/com/jinyuni/dengbei_care/VibrationPlayer.kt')])]

data = dict(started_utc=now(), source_head=git('rev-parse', 'HEAD'), initial_git_status=git('status', '--porcelain=v1'),
            cli_version=run([CLI, '--version']), container_has_index=(BASE/'.codegraph').is_dir(), roots=[])
OUT.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
for name, rel, queries in specs:
    root = BASE / rel
    record = dict(name=name, projectPath=str(root), indexPath=str(root/'.codegraph'),
                  exists=(root/'.codegraph').is_dir(), configuration=json.loads((root/'codegraph.json').read_text(encoding='utf-8-sig')),
                  subtree=git('rev-parse', 'HEAD:'+rel), last_root_commit=git('log', '-1', '--format=%H', '--', rel),
                  source_diff=git('diff', 'HEAD', '--', rel), queries=[])
    data['roots'].append(record)
    if not record['exists']:
        record['failure'] = 'Existing index missing; no automatic initialization'
        continue
    record['before'] = run([CLI, 'status', '--json', str(root)])
    record['sync'] = run([CLI, 'sync', str(root)], 60)
    record['after'] = run([CLI, 'status', '--json', str(root)])
    record['files'] = run([CLI, 'files', '-p', str(root), '--format', 'flat', '--no-metadata'])
    paths = [line.strip() for line in record['files']['stdout'].splitlines() if line.startswith('  ') and (root/line.strip()).is_file()]
    hashes = {path: hashlib.sha256((root/path).read_bytes()).hexdigest() for path in sorted(paths)}
    record['indexed_source_hashes'] = hashes
    record['source_set_sha256'] = hashlib.sha256(''.join(k+'\t'+v+'\n' for k,v in hashes.items()).encode('utf-8')).hexdigest()
    for query, count, target in queries:
        result = run([CLI, 'explore', '-p', str(root), '--max-files', str(count), query])
        raw = result.pop('stdout')
        lines = raw.splitlines()
        numbered = [(i,line) for i,line in enumerate(lines) if re.match(r'^\d+\t', line)]
        # Keep a small source excerpt around the named declaration, rather than the entire file.
        words = query.split() + ['processTemperatureHumidityData', 'stopAlarm', 'stopVibration', 'toggleButton', 'restoreGuardState', 'decode', 'measure_state']
        hits = [i for i,line in numbered if any(re.search(r'\b'+re.escape(word)+r'\b', line) for word in words)]
        hits = hits or [i for i,line in numbered[:1]]
        excerpt_ids = set()
        for i in hits[:5]:
            excerpt_ids.update(range(max(0,i-2), min(len(lines), i+13)))
        result.update(query=query, target=target, target_sha256=hashlib.sha256((root/target).read_bytes()).hexdigest(),
                      response_sha256=hashlib.sha256(raw.encode('utf-8')).hexdigest(),
                      source_headers=[line for line in lines if line.startswith('**`')],
                      call_context=[line for line in lines if line.startswith('- `')],
                      numbered_source_excerpt=[lines[i] for i in sorted(excerpt_ids) if re.match(r'^\d+\t',lines[i])],
                      current_source_located=('**`'+target+'`**' in raw and bool(numbered)))
        record['queries'].append(result)
    if name == 'android':
        for query in ['stopAlarm stopVibration', 'processTemperatureHumidityData startAlarm']:
            result = run([CLI, 'explore', '-p', str(root), '--max-files', '1', query])
            raw = result.pop('stdout')
            result.update(query=query, response_sha256=hashlib.sha256(raw.encode('utf-8')).hexdigest(),
                          source_headers=[x for x in raw.splitlines() if x.startswith('**`')],
                          call_context=[x for x in raw.splitlines() if x.startswith('- `')],
                          numbered_source_excerpt=[x for x in raw.splitlines() if re.match(r'^\d+\t',x)][:15])
            record.setdefault('supplementary_queries', []).append(result)
    record['tracked_index_files'] = git('ls-files', '--', rel+'/.codegraph')
    record['index_ignore'] = git('check-ignore', rel+'/.codegraph/')
    OUT.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
    print(name, record['after']['stdout'], 'queries', [q['current_source_located'] for q in record['queries']], flush=True)
data['ended_utc'] = now()
data['all_processes_reaped'] = True
OUT.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding='utf-8')
