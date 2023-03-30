#!/bin/bash

# 初始化计数器
total_lines=0

# 遍历文件夹中的所有文件
for file in $(find . -type f); do
  # 如果是文本文件，则统计行数并加到计数器中
  if [[ "$file" == *".txt" || "$file" == *".cpp" || "$file" == *".h" ]]; then
    num_lines=$(wc -l < $file)
    total_lines=$((total_lines + num_lines))
  fi
done

# 输出总行数
echo "Total lines of code: $total_lines"
