# ESPHome Makefile for Tab5 Dashboard

.DEFAULT_GOAL := help

help: ## Show this help text
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
.PHONY: help

compile: ## Compile firmware
	esphome compile tab5-dashboard.yaml
.PHONY: compile

upload: ## Upload firmware to device (OTA)
	esphome upload tab5-dashboard.yaml --device vision.local
.PHONY: upload

logs: ## View device logs
	esphome logs tab5-dashboard.yaml
.PHONY: logs

run: ## Compile, upload, and show logs (OTA)
	esphome run tab5-dashboard.yaml --device vision.local
.PHONY: run

clean: ## Clean build files
	esphome clean tab5-dashboard.yaml
.PHONY: clean

clean-all: ## Clean all build files
	rm -rf .esphome
.PHONY: clean-all

dashboard: ## Start ESPHome web dashboard
	esphome dashboard .
.PHONY: dashboard

validate: ## Validate config without compiling
	esphome config tab5-dashboard.yaml
.PHONY: validate

version: ## Show ESPHome version
	esphome version
.PHONY: version
