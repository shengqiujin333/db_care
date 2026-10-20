"""Bounded architecture experiments, no product changes. Run from any cwd."""
from pathlib import Path
import datetime, hashlib, json, subprocess, struct, sys

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
SENSOR = ROOT / 'CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output'
GW = ROOT / 'CH592EVT/EVT/EXAM/BLE/beiwov2/APP'
APP = ROOT / 'dengbei_care/app/src/main/java/com/jinyuni/dengbei_care'
records = []

def run(args, seconds):
    start = datetime.datetime.now(datetime.timezone.utc).isoformat()
    try:
        p = subprocess.run([str(x) for x in args], cwd=ROOT, capture_output=True, timeout=seconds)
        record = dict(command=[str(x) for x in args], started_utc=start,
                      timeout_sec=seconds, exit_code=p.returncode,
                      stdout=p.stdout.decode('utf-8', 'replace'), stderr=p.stderr.decode('utf-8', 'replace'))
    except (subprocess.TimeoutExpired, OSError) as exc:
        record = dict(command=[str(x) for x in args], started_utc=start, timeout_sec=seconds,
                      exit_code=None, error=str(exc))
    records.append(record)
    return record

sources = [SENSOR/'USER/src/encrytogate.c', GW/'feistel_al.c', GW/'bleencrypt.c',
           GW/'app_um2006A.c', APP/'protocol/GatewayFrameCodec.kt', APP/'MqtttService.kt',
           APP/'telemetry/AlarmEvaluator.kt', APP/'telemetry/ReadingAttribution.kt',
           APP/'cloud/CloudUploadRepository.kt', APP/'cloud/UploadPayload.kt',
           ROOT/'readme.txt', ROOT/'artifacts/product_specification.md',
           ROOT/'artifacts/firmware_exploration.md',
           GW.parent/'Profile/gattprofile.c',GW.parent/'Profile/include/gattprofile.h',
           APP/'ui/detail/DetailPresentation.kt', APP/'telemetry/SensorCadence.kt',
           ROOT/'artifacts/product_acceptance.md', ROOT/'artifacts/product_requirements.md',
           ROOT/'artifacts/product_scope.md', OUT/'probe.c', Path(__file__)]
identity = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
identity['git_head'] = run(['git','rev-parse','HEAD'],10).get('stdout','').strip()
run(['gcc','--version'],10)
exe = OUT/'probe.exe'
build = run(['gcc','-std=c11','-Wall','-Wextra','-I',SENSOR/'USER/inc','-I',GW,
             OUT/'probe.c',SENSOR/'USER/src/encrytogate.c',GW/'feistel_al.c',GW/'bleencrypt.c',
             '-o',exe],30)
model = {'classification':'local substitute adapters and constructed stimuli; not target evidence', 'cases':[]}
failed = build.get('exit_code') != 0
if not failed:
    probe = run([exe],10)
    failed = probe.get('exit_code') != 0
    for line in probe.get('stdout','').splitlines():
        if not line.startswith('VECTOR '): continue
        fields = dict(token.split('=',1) for token in line.split() if '=' in token)
        frame = bytes.fromhex(fields['plain'])
        # Reference adapter using exactly the reviewed Kotlin byte order and scale.
        hum, temp = struct.unpack('>Hh',frame[20:24])
        ok = temp==int(fields['input_t']) and hum==int(fields['input_h'])
        failed |= not ok
        model['cases'].append(dict(case=line.split()[1], frame_hex=frame.hex(),
                                  reference_android_temp=temp/10, reference_android_hum=hum/10, ok=ok))

# Identical cache frame read at different host times is assigned different keys.
model['freshness'] = dict(receive_seconds=[1000,1180], source_sample_identity='absent',
                         outbox_keys=[['12345678',1000],['12345678',1180]],
                         conclusion='transport retries can be idempotent; cache rereads are not identifiable')
# Normative gate, not current (known reversed) firmware logic.
prev = None
sequence=[]
for sec,temp,hum in [(0,300,600),(180,310,600),(360,280,500),(540,270,450)]:
    send=temp>350 or (prev is not None and temp-prev>9)  # all stimuli dark and valid
    sequence.append(dict(time=sec,temp_x10=temp,hum_x10=hum,report=send))
    prev=temp
model['drop_visibility'] = dict(stimulus=sequence, received_times=[x['time'] for x in sequence if x['report']],
                               conclusion='later cooling samples below 35C never reach the App; no decline observation')
# MQTT has no IDs: a permutation preserves counts and parser validity.
model['mqtt_permutation'] = dict(payload='1000:30,31:60,61', saved_ids_a=['A','B'], saved_ids_b=['B','A'],
                                accepted_by_count_check_both=True, conclusion='count equality cannot prove producer order')

# Registered read-only entry only. No install, app launch, BLE interaction or reset.
adb=Path('D:/software/platform-tools-latest-windows/platform-tools/adb.exe')
prefix=[adb,'-s','12f046690506']
state=run(prefix+['get-state'],10)
if state.get('exit_code')==0 and state.get('stdout','').strip()=='device':
    run(prefix+['shell','getprop','ro.product.model'],10)
    run(prefix+['shell','getprop','ro.build.version.sdk'],10)
    run(prefix+['shell','pm','list','packages','com.jinyuni.dengbei_care'],15)

(OUT/'execution.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'object_versions.json').write_text(json.dumps(identity,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'adapter_results.json').write_text(json.dumps(model,ensure_ascii=False,indent=2),encoding='utf-8')
snippets = [(GW/'app_um2006A.c',[(95,97),(167,226)]),
            (GW.parent/'Profile/gattprofile.c',[(365,378),(542,560)]),
            (GW.parent/'Profile/include/gattprofile.h',[(45,54)]),
            (APP/'MqtttService.kt',[(736,768),(818,850),(916,943),(984,999),(1278,1294)]),
            (APP/'protocol/GatewayFrameCodec.kt',[(112,158)]),
            (APP/'ui/detail/DetailPresentation.kt',[(31,39)])]
text=[]
for path,ranges in snippets:
    lines=path.read_text(encoding='utf-8',errors='replace').splitlines()
    text.append('\nFILE '+str(path.relative_to(ROOT)))
    for start,end in ranges:
        text.extend(f'{i}: {lines[i-1]}' for i in range(start,min(end,len(lines))+1))
(OUT/'source_excerpts.txt').write_text('\n'.join(text)+'\n',encoding='utf-8')
print(json.dumps(dict(local_probe_ok=not failed, commands=[dict(exit_code=r['exit_code'],command=r['command']) for r in records]),ensure_ascii=False))
sys.exit(1 if failed else 0)
