#!/bin/bash

echo "creating venv..."
python3.9 -m venv venv

echo "activating..."
source venv/bin/activate

echo "installing deps..."
pip install -r requirements.txt

echo "done"
