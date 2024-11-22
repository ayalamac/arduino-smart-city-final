install:
	@echo "=> installing application dependencies"
	@arduino-cli lib install "LiquidCrystal I2C"
	@arduino-cli core update-index
	@arduino-cli core install arduino:avr

.PHONY: compile
compile:
	@echo "=> compiling application"
	@arduino-cli compile --fqbn arduino:avr:mega --build-path ./build/ ./smart-city/
	@mv ./build/smart-city.ino.hex ./smart-city/
	@mv ./build/smart-city.ino.elf ./smart-city/



