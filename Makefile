# ─── Multi-Module Azure IoT Edge Pipeline ───
# Top-level Makefile for building, testing, and running modules

SHELL := /bin/bash
.DEFAULT_GOAL := help

# ─── Configuration ───
REGISTRY ?= localhost:5000
VERSION  ?= 1.0.0

# ─── Directories ───
SENSOR_DIR   := modules/sensor_simulator
FILTER_DIR   := modules/data_filter
ANALYTICS_DIR := modules/analytics_alert

# ─── Colors ───
GREEN  := \033[0;32m
YELLOW := \033[0;33m
CYAN   := \033[0;36m
RESET  := \033[0m

.PHONY: help build build-sensor build-filter build-analytics \
        docker docker-sensor docker-filter docker-analytics \
        test test-analytics clean run-local run-pipeline

# ─── Help ───
help:
	@echo ""
	@echo "$(CYAN)Multi-Module Azure IoT Edge Pipeline$(RESET)"
	@echo ""
	@echo "$(GREEN)Build Commands:$(RESET)"
	@echo "  make build              Build all C++ modules locally (standalone mode)"
	@echo "  make build-sensor       Build sensor_simulator"
	@echo "  make build-filter       Build data_filter"
	@echo ""
	@echo "$(GREEN)Docker Commands:$(RESET)"
	@echo "  make docker             Build all Docker images"
	@echo "  make docker-sensor      Build sensor_simulator image"
	@echo "  make docker-filter      Build data_filter image"
	@echo "  make docker-analytics   Build analytics_alert image"
	@echo ""
	@echo "$(GREEN)Test Commands:$(RESET)"
	@echo "  make test               Run all tests"
	@echo "  make test-analytics     Run Python analytics tests"
	@echo ""
	@echo "$(GREEN)Run Commands:$(RESET)"
	@echo "  make run-local          Run full pipeline locally (pipe mode)"
	@echo "  make run-sensor         Run sensor_simulator standalone"
	@echo "  make run-pipeline       Run pipeline via Docker Compose"
	@echo ""
	@echo "$(GREEN)Other:$(RESET)"
	@echo "  make clean              Remove all build artifacts"
	@echo "  make setup              Install local dependencies"
	@echo ""

# ─── Local Build (Standalone Mode) ───
build: build-sensor build-filter
	@echo "$(GREEN)All C++ modules built successfully$(RESET)"

build-sensor:
	@echo "$(CYAN)Building sensor_simulator...$(RESET)"
	@mkdir -p $(SENSOR_DIR)/build
	@cd $(SENSOR_DIR)/build && cmake .. -DCMAKE_BUILD_TYPE=Release -DSTANDALONE_MODE=ON && make -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc)
	@echo "$(GREEN)sensor_simulator built$(RESET)"

build-filter:
	@echo "$(CYAN)Building data_filter...$(RESET)"
	@mkdir -p $(FILTER_DIR)/build
	@cd $(FILTER_DIR)/build && cmake .. -DCMAKE_BUILD_TYPE=Release -DSTANDALONE_MODE=ON && make -j$$(sysctl -n hw.ncpu 2>/dev/null || nproc)
	@echo "$(GREEN)data_filter built$(RESET)"

# ─── Docker Build ───
docker: docker-sensor docker-filter docker-analytics
	@echo "$(GREEN)All Docker images built$(RESET)"

docker-sensor:
	@echo "$(CYAN)Building sensor-simulator Docker image...$(RESET)"
	docker build -t $(REGISTRY)/sensor-simulator:$(VERSION) $(SENSOR_DIR)

docker-filter:
	@echo "$(CYAN)Building data-filter Docker image...$(RESET)"
	docker build -t $(REGISTRY)/data-filter:$(VERSION) $(FILTER_DIR)

docker-analytics:
	@echo "$(CYAN)Building analytics-alert Docker image...$(RESET)"
	docker build -t $(REGISTRY)/analytics-alert:$(VERSION) $(ANALYTICS_DIR)

# ─── Tests ───
test: test-analytics
	@echo "$(GREEN)All tests passed$(RESET)"

test-analytics:
	@echo "$(CYAN)Running analytics tests...$(RESET)"
	@cd $(ANALYTICS_DIR) && python3 tests/test_analytics.py

# ─── Run Locally ───
run-sensor: build-sensor
	@echo "$(CYAN)Running sensor_simulator...$(RESET)"
	$(SENSOR_DIR)/build/sensor_simulator

run-local: build
	@echo "$(CYAN)Running full pipeline (sensor | filter | analytics)...$(RESET)"
	@echo "$(YELLOW)Press Ctrl+C to stop$(RESET)"
	$(SENSOR_DIR)/build/sensor_simulator | $(FILTER_DIR)/build/data_filter | python3 $(ANALYTICS_DIR)/src/main.py

run-pipeline:
	@echo "$(CYAN)Starting Docker Compose pipeline...$(RESET)"
	docker compose -f docker-compose.pipeline.yml up --build

# ─── Clean ───
clean:
	@echo "$(CYAN)Cleaning build artifacts...$(RESET)"
	@rm -rf $(SENSOR_DIR)/build
	@rm -rf $(FILTER_DIR)/build
	@rm -rf $(ANALYTICS_DIR)/src/__pycache__
	@rm -rf $(ANALYTICS_DIR)/tests/__pycache__
	@echo "$(GREEN)Clean complete$(RESET)"

# ─── Setup ───
setup:
	@echo "$(CYAN)Checking prerequisites...$(RESET)"
	@command -v cmake >/dev/null 2>&1 || { echo "$(YELLOW)cmake not found. Install: brew install cmake$(RESET)"; exit 1; }
	@command -v docker >/dev/null 2>&1 || { echo "$(YELLOW)docker not found. Install Docker Desktop$(RESET)"; exit 1; }
	@command -v python3 >/dev/null 2>&1 || { echo "$(YELLOW)python3 not found$(RESET)"; exit 1; }
	@echo "$(GREEN)All prerequisites met$(RESET)"
	@echo "$(CYAN)Copying .env template...$(RESET)"
	@cp -n .env.template .env 2>/dev/null || true
	@echo "$(GREEN)Setup complete. Edit .env with your Azure credentials.$(RESET)"
