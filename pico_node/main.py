import random
import json
from lib.sx1262 import SX1262
import time

import ubinascii
import machine
import dht
import uuid

from logging import Logger

# GLOBAL_ID = "ID_{}".format(random.randint(0, 1000))
GLOBAL_ID = "ID_{}".format(ubinascii.hexlify(machine.unique_id()).decode('utf-8'))
print("=========================================")
print("SX1262 LoRa MESH SYSTEM BY JBYLINA")
print(GLOBAL_ID)
print("=========================================")

random_delay = random.randint(0, 1000)
def check_msg(msg: str):
    try:
        msg = json.loads(msg)
    except ValueError:
        return False
    if "d_id" not in msg:
        return False
    if "m_id" not in msg:
        return False
    if "ttl" not in msg:
        return False
    if "values" not in msg:
        return False
    return True


def handel_msg(msg: bytes):
    logger = Logger()
    msg = msg.decode('utf-8')
    logger.log_info("Message received: {}".format(msg))
    if not check_msg(msg):
        logger.log_error("Message is not valid")
        return

    try:
        msg = json.loads(msg)
    except ValueError:
        logger.log_error("Message is not JSON")
        return

    if msg["d_id"] == GLOBAL_ID:
        logger.log_warning("Message from myself !!!")
        return

    if msg["ttl"] == 0:
        logger.log_warning(f"Message dropped: {msg}, TTL is 0")
        return
    
    if msg["ttl"] > 0:

        msg["ttl"] -= 1
        msg = json.dumps(msg)
        msg = msg.encode('utf-8')
        time.sleep(random_delay/1000.0)
        sx.send(msg)
        logger.log_info(f"Message forwarded: {msg}")

def cb(events):
    logger = Logger()
    if events & SX1262.RX_DONE:
        msg, err = sx.recv()
        error = SX1262.STATUS[err]
        if error != 'ERR_NONE':
            logger.log_error('RX error: {}'.format(error))
        else:
            handel_msg(msg)

    elif events & SX1262.TX_DONE:
        logger.log_debug('TX done.')

logger = Logger()
logger.log_info("Starting...")
logger.log_info("Initializing SX1262...")
logger.log_info("TEST LOG INFO GREEN")
logger.log_warning("TEST LOG WARNING YELLOW")
logger.log_error("TEST LOG ERROR RED")

sx = SX1262(spi_bus=1, clk=10, mosi=11, miso=12, cs=3, irq=20, rst=15, gpio=2)

sx.begin(freq=868, bw=125.0, sf=7, cr=5, syncWord=0x12,
         power=22, currentLimit=140.0, preambleLength=8,
         implicit=False, implicitLen=0xFF,
         crcOn=True, txIq=False, rxIq=False,
         tcxoVoltage=1.7, useRegulatorLDO=False, blocking=False)

sx.setBlockingCallback(False, cb)



d = dht.DHT11(machine.Pin(14))
# time.sleep(1)
while True:
    # rand = random.randint(0, 1000)


    message_id = str(uuid.uuid4())
    # logger.log_info("Message ID: {}".format(message_id))

    logger.log_info("Measuring...")
    d.measure()
    logger.log_info("Temperature: {} C".format(d.temperature()))
    logger.log_info("Humidity: {} %".format(d.humidity()))

    data = {"d_id": GLOBAL_ID,
            "m_id": message_id,
            "values": {
                "humidity": d.humidity(),
                "temperature": d.temperature(),
            },
            "ttl": 10}
    data = json.dumps(data)

    sx.send(data.encode())

    logger.log_info("Sending: {}".format(data))
    time.sleep(300)
