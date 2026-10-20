"""Static contract/schema/example check only; no devices, build or network."""
from pathlib import Path
import datetime, hashlib, json, re, struct, subprocess

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'evidence'
checks=[]
def check(name, condition):
    checks.append({'name':name,'ok':bool(condition)})

def crc(data, width, poly, init):
    v=init; mask=(1<<width)-1
    for byte in data:
        v ^= byte << (width-8)
        for _ in range(8):
            v=((v<<1)^poly if v & (1<<(width-1)) else v<<1)&mask
    return v

doc=(ROOT/'artifacts/interface_contract_v4.md').read_text(encoding='utf-8')
check('IF01..IF08 present',all(f'IF{i:02}' in doc for i in range(1,9)))
check('PACC-002 and PA-026..031 referenced', 'PACC-002' in doc and all(f'{i:03}' in doc for i in range(26,32)))
check('UTF-8 and balanced code fences','\ufffd' not in doc and doc.count('```')%2==0)
check('CRC8 documented vector',crc(bytes.fromhex('BEEF'),8,0x31,0xff)==0x92)
check('CRC16 standard vector',crc(b'123456789',16,0x1021,0xffff)==0x29B1)

vectors=[]
for uid,t,h in [('01020304',-50,600),('12345678',0,0),('12345678',351,1000)]:
    prefix=bytes.fromhex(uid)+struct.pack('<hH',t,h)
    full=prefix+struct.pack('<H',crc(prefix,16,0x1021,0xffff))
    block=bytes.fromhex(uid)+struct.pack('>Hh',h,t)
    vectors.append({'id':uid,'temperature_x10':t,'humidity_x10':h,
                    'rf_plain_hex':full.hex().upper(),'ble_payload8_hex':block.hex().upper()})
    check(f'{uid} {t}/{h} RF layout',len(full)==10 and struct.unpack('<hH',full[4:8])==(t,h))
    check(f'{uid} {t}/{h} BLE layout',struct.unpack('>Hh',block[4:8])==(h,t))
check('documented cross-endian vector',vectors[0]['ble_payload8_hex']=='010203040258FFCE'
      and vectors[0]['rf_plain_hex'].startswith('01020304CEFF5802'))
check('zero preserved in schema',vectors[1]['temperature_x10']==0 and vectors[1]['humidity_x10']==0)
# Length and boundary examples are reference arithmetic, not execution of Kotlin/MCU.
lengths=[{'count':n,'effective_bytes':16*(1+n),'carrier_bytes':160,
          'fits_carrier':16*(1+n)<=160} for n in [1,2,9,10]]
check('BLE effective vs carrier lengths',[v['effective_bytes'] for v in lengths]==[32,48,160,176])
check('BLE ten devices rejected in 160B',not lengths[-1]['fits_carrier'])
light=[{'c_dark':4000,'mean':1333,'dark':False},{'c_dark':4000,'mean':1334,'dark':True}]
check('light integer boundary',all((3*x['mean']>=x['c_dark'])==x['dark'] for x in light))
report_cases=[(300,308,True,True,True,False),(300,309,True,True,True,False),
              (300,310,True,True,True,True),(300,310,True,True,False,False),
              (300,310,True,False,False,False),(310,300,True,True,True,False),
              (340,350,True,True,True,True),(340,350,True,True,False,False),
              (0,351,False,True,False,True),(0,350,False,True,True,False),
              (0,360,False,True,False,True),(352,351,True,True,False,True),
              (300,310,True,False,False,False),(300,310,True,True,True,False)]
gate=[]
for i,(p,t,have,lv,dark,expected) in enumerate(report_cases,1):
    valid=i!=14
    actual=valid and (t>350 or (have and lv and dark and t-p>9))
    check(f'PDR gate vector {i}',actual==expected)
    gate.append(dict(case=i,previous_x10=p,current_x10=t,have_previous=have,
                     light_valid=lv,dark=dark,sample_valid=valid,report=expected))

sample='S k=4294967295 p=-400 H=1 V=1 o=8 n=4095 x=4095 a=4095 D=1 t=1250 h=1000 q=1 r=1 s=2 y=3\r\n'
gline='G s=2 a44=0 a45=0 rd=5 at=3 raw=------------\r\n'
check('S line budget including CRLF',len(sample.encode('ascii'))<=96)
check('G line budget including CRLF',len(gline.encode('ascii'))<=96)
upload={'phone':'已注册手机号','mac':'已确认网关MAC','readings':[{'devId':'01020304',
        'time':1700000000,'temperature':-5.0,'humidity':60.0}]}
check('HTTP field sets',set(upload)=={'phone','mac','readings'} and
      set(upload['readings'][0])=={'devId','time','temperature','humidity'})
check('time integer not rescaled',isinstance(upload['readings'][0]['time'],int))

sensor=ROOT/'CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER'
app=ROOT/'dengbei_care/app/src/main/java/com/jinyuni/dengbei_care'
gw=ROOT/'CH592EVT/EVT/EXAM/BLE/beiwov2'
source_paths=[sensor/'inc/gxht40.h',sensor/'inc/light.h',sensor/'inc/debug_trace.h',
              sensor/'inc/sensor_config.h',sensor/'src/fw_core.c',sensor/'src/measure.c',
              sensor/'src/debug_trace.c',sensor/'src/encrytogate.c',gw/'APP/feistel_al.c',
              gw/'APP/bleencrypt.c',gw/'APP/app_um2006A.c',gw/'Profile/include/gattprofile.h',
              gw/'Profile/gattprofile.c',app/'protocol/GatewayFrameCodec.kt',
              app/'telemetry/ReadingAttribution.kt',app/'MqtttService.kt',
              app/'cloud/UploadOutbox.kt',app/'cloud/CloudUploadRepository.kt',
              app/'cloud/UploadPayload.kt',app/'cloud/CloudConfig.kt',
              app/'ui/zhuce/ApiService.kt',ROOT/'服务器迁移记录.md',
              ROOT/'artifacts/product_acceptance_v2.md',ROOT/'artifacts/system_decomposition.md',
              ROOT/'artifacts/system_architecture.md',ROOT/'artifacts/architecture_exploration.md',
              ROOT/'artifacts/product_requirements.md',ROOT/'artifacts/product_specification.md',
              ROOT/'artifacts/interface_contract_v4.md',Path(__file__)]
hashes={str(p.relative_to(ROOT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths}
check('current light API', 'light_result_t light_sample(void)' in (sensor/'inc/light.h').read_text(encoding='utf-8'))
check('current fixed GATT length',bool(re.search(r'#define\s+SIMPLEPROFILE_CHAR2_LEN\s+160',
      (gw/'Profile/include/gattprofile.h').read_text(encoding='utf-8'))))
check('current codec allows effective prefix','frame.size < expected' in (app/'protocol/GatewayFrameCodec.kt').read_text(encoding='utf-8'))
check('current upload DTO fields','data class SensorReading' in (app/'ui/zhuce/ApiService.kt').read_text(encoding='utf-8'))
fixture=dict(classification='Static schema/example arithmetic; no product implementation execution',
             vectors=vectors,ble_lengths=lengths,light_boundaries=light,gate_vectors=gate,
             uart_examples=[sample,gline],upload_example=upload)
(OUT/'interface_contract_v4_fixtures.json').write_text(json.dumps(fixture,ensure_ascii=False,indent=2),encoding='utf-8')
head=subprocess.run(['git','rev-parse','HEAD'],cwd=ROOT,capture_output=True,timeout=10)
check('git identity command exited',head.returncode==0)
record=dict(kind='contract author static consistency only; not independent product validation',
            baseline=head.stdout.decode('utf-8','replace').strip(),
            checked_at_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
            input_sha256=hashes,checks=checks,passed=sum(x['ok'] for x in checks),total=len(checks))
(OUT/'interface_contract_v4_check.json').write_text(json.dumps(record,ensure_ascii=False,indent=2),encoding='utf-8')
print(f"STATIC CHECKS {record['passed']}/{record['total']}")
print('RF fixture:',vectors[0]['rf_plain_hex'])
print('UART bytes:',len(sample.encode('ascii')),len(gline.encode('ascii')))
raise SystemExit(0 if all(c['ok'] for c in checks) else 1)
