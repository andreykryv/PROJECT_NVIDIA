#!/bin/bash
# Сбор информации о системе для QWEN.md
# Запускать: ./collect_env.sh

echo "============================================="
echo "  СБОР ДАННЫХ ДЛЯ QWEN.md  "
echo "============================================="

# 1. Имя проекта и исполняемый файл (спросим)
read -p "Введите имя проекта (PROJECT_NAME): " PROJECT_NAME
read -p "Введите имя целевого исполняемого файла (EXECUTABLE_NAME): " EXECUTABLE_NAME
echo ""

# 2. GCC / Clang
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc -dumpversion)
    echo "✅ GCC версия: $GCC_VERSION"
else
    echo "❌ GCC не найден"
fi

if command -v clang &> /dev/null; then
    CLANG_VERSION=$(clang --version | head -n1 | grep -oP '\d+\.\d+\.\d+')
    echo "   Clang версия: $CLANG_VERSION"
fi

# 3. CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    echo "✅ CMake версия: $CMAKE_VERSION"
else
    echo "❌ CMake не найден"
fi

# 4. Qt6
QT6_PATH=""
if command -v qmake6 &> /dev/null; then
    QT6_VERSION=$(qmake6 -query QT_VERSION)
    QT6_PATH=$(qmake6 -query QT_INSTALL_PREFIX)
    echo "✅ Qt6 версия: $QT6_VERSION"
    echo "   Префикс установки: $QT6_PATH"
elif pkg-config --exists Qt6Core 2>/dev/null; then
    QT6_VERSION=$(pkg-config --modversion Qt6Core)
    QT6_PATH=$(pkg-config --variable=prefix Qt6Core)
    echo "✅ Qt6 версия (pkg-config): $QT6_VERSION"
    echo "   Префикс установки: $QT6_PATH"
else
    echo "❌ Qt6 не найден. Установите qt6-base"
fi

# 5. CUDA Toolkit
CUDA_VERSION=""
if command -v nvcc &> /dev/null; then
    CUDA_VERSION=$(nvcc --version | grep -oP 'release \K\d+\.\d+')
    CUDA_PATH=$(dirname $(dirname $(which nvcc)))
    echo "✅ CUDA Toolkit версия: $CUDA_VERSION"
    echo "   Путь: $CUDA_PATH"
else
    echo "❌ nvcc не найден. Установите cuda"
fi

# 6. Compute Capability (GPU)
echo ""
echo "--- GPU информация ---"
if command -v nvidia-smi &> /dev/null; then
    GPU_NAME=$(nvidia-smi --query-gpu=name --format=csv,noheader)
    COMPUTE_CAP=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader)
    echo "✅ GPU: $GPU_NAME"
    echo "   Compute Capability: $COMPUTE_CAP"
else
    echo "❌ nvidia-smi не найден. Проверьте установку драйверов NVIDIA."
fi

# 7. Дополнительные инструменты
echo ""
echo "--- Инструменты разработки ---"
for tool in cppcheck clangd gdb cmake-format; do
    if command -v $tool &> /dev/null; then
        echo "✅ $tool доступен"
    else
        echo "❌ $tool отсутствует"
    fi
done

# Итоговый блок для копирования
echo ""
echo "============================================="
echo "  ГОТОВЫЕ ПОДСТАНОВКИ ДЛЯ QWEN.md  "
echo "============================================="
cat <<EOF
__PROJECT_NAME__        ->  $PROJECT_NAME
__QT_VERSION__          ->  ${QT6_VERSION:-УКАЖИТЕ_ВЕРСИЮ}
__CUDA_VERSION__        ->  ${CUDA_VERSION:-УКАЖИТЕ_ВЕРСИЮ}
__CMAKE_MIN_VERSION__   ->  ${CMAKE_VERSION:-3.20}
__GCC_VERSION__         ->  ${GCC_VERSION:-УКАЖИТЕ_ВЕРСИЮ}
__COMPUTE_CAP__         ->  ${COMPUTE_CAP:-УКАЖИТЕ_CAP}
__QT6_PATH__            ->  ${QT6_PATH:-/usr}
__EXECUTABLE_NAME__     ->  $EXECUTABLE_NAME
EOF

echo ""
echo "Скопируйте эти значения в QWEN.md, а также проверьте архитектуру и соглашения."