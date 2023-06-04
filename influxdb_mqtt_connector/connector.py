import logging
from time import sleep

import paho.mqtt.client
import redis

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import json

import os

logging.basicConfig(
    format='%(asctime)s %(levelname)-8s %(message)s',
    level=logging.INFO,
    datefmt='%Y-%m-%d %H:%M:%S')


def parse_json_message(msg) -> dict | None:
    """
    This method is used to parse json message to dict.
    :param msg: message to parse
    :return: dict with parsed message
    """
    try:
        json_msg = json.loads(msg.payload.decode("utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        logging.log(logging.ERROR, "Invalid json message")
        return None
    # logging.log(logging.INFO, "Parsed message: " + str(json_msg))
    return json_msg


class Connector:
    """
    This class is used to consume data from MQTT and send it to the Influxdb.
    """

    def __init__(self,
                 mqtt_host: str, mqtt_user: str, mqtt_password: str, mqtt_topic: str,
                 influxdb_host: str, influxdb_token: str, influxdb_org: str, influxdb_bucket: str,
                 redis_host: str,
                 influxdb_port: int = 8086, mqtt_port: int = 1883, redis_port: int = 6379, redis_db: int = 0,
                 redis_password: str = None,
                 redis_expire: int = 60 * 60
                 ):
        """
        This method is used to initialize the class.
        :param mqtt_host:
        :param mqtt_user:
        :param mqtt_password:
        :param influxdb_host:
        :param influxdb_token:
        :param influxdb_org:
        :param influxdb_port: default 8086
        :param mqtt_port: default 1883
        """
        self.mqtt_host = mqtt_host
        self.mqtt_port = mqtt_port
        self.mqtt_topic = mqtt_topic

        self.influxdb_host = influxdb_host
        self.influxdb_port = influxdb_port
        self.influxdb_token = influxdb_token
        self.influxdb_org = influxdb_org

        # create influxdb client
        self.influxdb_client = InfluxDBClient(url=f"http://{self.influxdb_host}:{self.influxdb_port}",
                                              token=self.influxdb_token,
                                              org=self.influxdb_org)

        self.influxdb_bucket = influxdb_bucket

        # create mqtt client
        self.mqtt_client = paho.mqtt.client.Client()
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        self.mqtt_client.username_pw_set(mqtt_user, mqtt_password)
        self.mqtt_client.reconnect_delay_set(min_delay=1, max_delay=120)
        self.mqtt_client.on_disconnect = self.reconnect_if_needed

        self.redis_client = redis.Redis(
            host=redis_host, port=redis_port, db=redis_db, password=redis_password)

        self.REDIS_EXPIRE = redis_expire

    def on_connect(self, client, userdata, msg, rc):
        logging.log(
            logging.INFO, "Connected to mqtt with result code " + str(rc))
        client.subscribe(self.mqtt_topic)

    def on_message(self, client, userdata, msg):
        try:
            logging.log(logging.INFO, "Received message: " +
                        msg.payload.decode("utf-8"))
        except UnicodeDecodeError:
            logging.log(logging.WARN, "Received message: " +
                        msg.payload.decode("utf-8", "ignore"))

        self.process_message(msg)

    def start(self):
        """
        This method is used to start connector thread..
        :return:
        """
        self.mqtt_client.connect(self.mqtt_host, self.mqtt_port)

        self.mqtt_client.loop_start()

    def stop(self):
        """
        This method is used to stop connector thread.
        :return:
        """
        self.mqtt_client.loop_stop()
        logging.log(logging.INFO, "Connector stopped.")

    def save_to_influxdb(self, json_msg):
        write_api = self.influxdb_client.write_api(write_options=SYNCHRONOUS)
        for key, value in json_msg["values"].items():
            point = Point(key).tag(
                "device", json_msg["d_id"]).field("value", value)
            write_api.write(bucket=self.influxdb_bucket, record=point)

    def save_to_redis(self, json_msg):
        self.redis_client.set(
            json_msg["m_id"], "PROCESSED", ex=self.REDIS_EXPIRE)

    def process_message(self, msg):
        json_msg = parse_json_message(msg)
        if json_msg and self.valide_message(json_msg) and not self.check_if_processed(json_msg["m_id"]):
            self.save_to_redis(json_msg)
            self.save_to_influxdb(json_msg)

    def valide_message(self, json_msg: dict) -> bool:
        if "d_id" not in json_msg:
            return False

        if "m_id" not in json_msg:
            return False

        if "values" not in json_msg or not isinstance(json_msg["values"], dict):
            return False

        if "ttl" not in json_msg:
            return False

        return True

    def check_if_processed(self, message_id: str) -> bool:
        """
        This method is used to check if message was already processed. This is done by checking if message is in the REDIS.
        :param message_id:
        :return:
        """
        # check if message is in redis
        if self.redis_client.get(message_id):
            logging.log(logging.INFO, "Message already processed")
            return True
        return False

    def reconnect_if_needed(self):
        if not self.mqtt_client.is_connected():
            logging.log(logging.INFO, "Reconnecting to mqtt")
            self.mqtt_client.reconnect()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    logging.log(logging.INFO, "Starting connector...")

    connector = Connector(mqtt_host=os.getenv("MQTT_HOST", "mqtt"),
                          mqtt_port=os.getenv("MQTT_PORT", 1883),
                          mqtt_user=os.getenv("MQTT_USER", "test"),
                          mqtt_topic=os.getenv("MQTT_TOPIC", "test"),
                          mqtt_password=os.getenv("MQTT_PASSWORD", "test")
                          influxdb_host=os.getenv("INFLUXDB_HOST", "influxdb"),
                          influxdb_port=os.getenv("INFLUXDB_PORT", 8086),
                          influxdb_token=os.getenv("DB_TOKEN"),
                          influxdb_org=os.getenv("DB_ORG", "test"),
                          redis_host=os.getenv("REDIS_HOST", "redis"),
                          influxdb_bucket=os.getenv("DB_BUCKET", "test"))
    connector.start()
    while True:
        try:
            sleep(1)
        except KeyboardInterrupt:
            logging.log(logging.INFO, "Stopping connector...")
            connector.stop()

            logging.log(logging.INFO, "Exiting...")
            break
