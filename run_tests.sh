#!/bin/bash

# WebServer测试运行脚本

echo "WebServer Test Runner"
echo "===================="

# 检查虚拟环境是否存在
if [ ! -d "venv" ]; then
    echo "Virtual environment not found. Creating one..."
    python3 -m venv venv
fi

# 激活虚拟环境
echo "Activating virtual environment..."
source venv/bin/activate

# 检查是否有新的依赖需要安装
echo "Checking for required packages..."
pip install -r requirements.txt > /dev/null 2>&1

# 显示pytest版本
echo "Pytest version: $(python -m pytest --version | head -n1)"

# 运行测试
echo "Running tests..."
echo "----------------"

if [ "$#" -eq 0 ]; then
    # 默认运行所有测试
    python -m pytest tests/ -v
elif [ "$1" = "integration" ]; then
    # 运行集成测试
    python -m pytest tests/integration_test/ -v
elif [ "$1" = "concurrency" ]; then
    # 运行并发测试
    python tests/test_concurrency.py
elif [ "$1" = "flood" ]; then
    # 运行连接洪泛测试
    python tests/test_connection_flood.py
else
    # 运行指定测试
    python -m pytest "$@" -v
fi

# 退出虚拟环境
deactivate

echo ""
echo "Test run completed."