# ===== CONFIG =====

PYTHON_VERSION=3.9.7
VENV=venv

# ===== DEFAULT =====

all: setup run

# ===== INSTALL SYSTEM DEPS =====

deps:
	sudo apt update
	sudo apt install -y build-essential curl libssl-dev zlib1g-dev libbz2-dev libreadline-dev libsqlite3-dev liblzma-dev tk-dev libxcb-xinerama0 libxkbcommon-x11-0 libxcb-cursor0 libx11-xcb1 libxcb1 libxcb-util1 libxcb-image0 libxcb-icccm4 libxcb-keysyms1 libxcb-render-util0 xauth

# ===== INSTALL PYENV =====

pyenv:
	curl https://pyenv.run | bash
	echo 'export PATH="$$HOME/.pyenv/bin:$$PATH"' >> ~/.bashrc
	echo 'eval "$$(pyenv init --path)"' >> ~/.bashrc
	echo 'eval "$$(pyenv init -)"' >> ~/.bashrc

# ===== INSTALL PYTHON =====

python:
	~/.pyenv/bin/pyenv install -s $(PYTHON_VERSION)
	~/.pyenv/bin/pyenv local $(PYTHON_VERSION)

# ===== CREATE VENV =====

venv:
	python -m venv $(VENV)

# ===== INSTALL PYTHON LIBS =====

install:
	. $(VENV)/bin/activate && 
	pip install --upgrade pip && 
	pip install pyqt5==5.15.9 pyserial==3.5 && 
	pip freeze > requirements.txt

# ===== FULL SETUP =====

setup: deps pyenv python venv install

# ===== RUN APP =====

run:
	. $(VENV)/bin/activate && python esp32_modbus.py

# ===== HEADLESS RUN =====

run-headless:
	. $(VENV)/bin/activate && QT_QPA_PLATFORM=offscreen python esp32_modbus.py

# ===== CLEAN =====

clean:
	rm -rf $(VENV) requirements.txt

re: clean setup