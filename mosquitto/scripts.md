```bash
mkdir log data
touch config/passwd
sudo chown -R 1883:1883 config data log
docker compose up -d
docker compose exec mosquitto mosquitto_passwd -b /mosquitto/config/passwd hmb1604 1604
sudo chown root:root config/passwd
sudo chmod 0600 config/passwd
docker compose restart
docker compose logs -f
docker exec -it mosquitto mosquitto_sub -t "test/topic" -u "" -P "" -v
docker exec -it mosquitto mosquitto_pub -t "test/topic" -m "" -u "" -P ""
```
