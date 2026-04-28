.DEFAULT_GOAL := upload

upload:
	pio run -t upload

debug:
	pio device monitor
