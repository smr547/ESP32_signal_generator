# build and upload the binary

arduino-cli compile -v --fqbn esp32:esp32:esp32wrover ./src/ESP_signal_generator/ESP_signal_generator.ino

arduino-cli upload -p /dev/ttyUSB2 --fqbn esp32:esp32:esp32wrover:UploadSpeed=115200 ./src/ESP_signal_generator/ESP_signal_generator.ino
