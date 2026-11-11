import hashlib
import zipfile

p = "fw_release/fw_sensor_FD-002r8.zip"
d = open(p, "rb").read()
z = zipfile.ZipFile(p)
print("package   :", p)
print("size      :", len(d))
print("md5       :", hashlib.md5(d).hexdigest())
bad = z.testzip()
print("crc_test  :", "OK (no bad member)" if bad is None else bad)
for i in z.infolist():
    b = z.read(i.filename)
    print("member    : {} {} B md5={} crc={:08X}".format(
        i.filename, len(b), hashlib.md5(b).hexdigest(), i.CRC))
