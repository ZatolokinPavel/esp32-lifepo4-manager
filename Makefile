.DEFAULT_GOAL := upload

build:
	pio run

upload:
	pio run -t upload

debug:
	pio device monitor
