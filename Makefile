# Docker Manager Build System

.DEFAULT_GOAL := all

.PHONY: metainstaller baseimage web

# Default target
all: web metainstaller
	@echo "createing web package then metainstaller file"

metainstaller:
	docker compose --env-file ".env.baseimage" --env-file ".env.version" up builder

web:
	docker compose --env-file ".env.baseimage" --env-file ".env.version" up --build web_build

baseimage:
	bash create_baseimage.sh

help:
	@echo "Docker Manager Build System"
	@echo "run 'make' to create MetaInstaller static app."
	@echo "run 'make baseimage' to create MetaInstaller base docker image."
	@echo "run 'make web' to create web build directory."
