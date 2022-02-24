import wiringpi
import time

PIN = 3
DELAY_70_HZ = int(10 ** 6 / 70)

wiringpi.wiringPiSetup()
wiringpi.pinMode(PIN, wiringpi.INPUT)

start = time.time()
cnt = 0
last = None
while True:
    now = wiringpi.digitalRead(PIN)
    if last is not None and now != last:
        cnt += 1
    last = now

    now = time.time()
    if now - start > 1:
        print(f"{now:.02f} {cnt}")
        cnt = 0
        start = now

    wiringpi.delayMicroseconds(DELAY_70_HZ)
