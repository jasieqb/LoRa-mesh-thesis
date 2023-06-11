# connector

InfluxDB MQTT connector, written in Python. 
Connects to InfluxDB and subscribes to MQTT topics. When a message is received, it is parsed and saved to InfluxDB. 
Contains docker-compose file for easy deployment.

## Run

1. Install [Docker](https://docs.docker.com/get-docker/) and [Docker Compose](https://docs.docker.com/compose/install/)
2. Fill necessary environment variables in `docker-compose.yml`
3. Run `docker-compose up -d`
