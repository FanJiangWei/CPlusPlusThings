# -*- coding: utf-8 -*-

import os
import struct
import zlib

# 定义结构体中各字段的长度
VERSION_LEN = 2
CHIP_LEN = 1
DEVICETYPE_LEN = 1
IDENT_LEN = 10
EXT_FLAG_LEN = 1
RES_LEN = 1
OUTVERSION_LEN = 2
OUTDATE_LEN = 2
CRC_LEN = 4
SZ_FILE_LEN = 4
SZ_IH_LEN = 4
SZ_SH_LEN = 4
NR_SH_LEN = 4

def replace_bytes(file_path, replace_data, position_index, data_name):
    # 读取原始的指定字节
    with open(file_path, "r+b") as f:
        f.seek(position_index)
        original_bytes = f.read(len(replace_data))

    # 打印原始的指定字节（十六进制，空格连接）
    print(f"原始{data_name}：{' '.join(f'{byte:02X}' for byte in original_bytes)}")

    # 将新的字节写回文件中
    with open(file_path, "r+b") as f:
        f.seek(position_index)
        f.write(replace_data)

    # 读取替换后的指定字节
    with open(file_path, "r+b") as f:
        f.seek(position_index)
        replaced_bytes = f.read(len(replace_data))

    # 打印替换后的指定字节（十六进制，空格连接）
    print(f"替换后{data_name}：{' '.join(f'{byte:02X}' for byte in replaced_bytes)}")

# 获取脚本所在目录的绝对路径
script_dir = os.path.dirname(os.path.abspath(__file__))

# 获取目录中以大写字母ZC开头的所有BIN文件
bin_files = [file for file in os.listdir(script_dir) if file.endswith(".bin") and file.startswith("ZC")]

# 处理文件
for file in bin_files:
    file_path = os.path.join(script_dir, file)
    
    # 添加文件存在性检查
    if os.path.exists(file_path):
        file_size = os.path.getsize(file_path)

        # 计算文件大小与4096的余数
        remainder = file_size % 4096

        # 判断是否需要在文件末尾补零
        if remainder > (4096 - 512):
            padding_size = 4096 - remainder + 4  # Ensure the remainder is at least 4 after padding
            with open(file_path, "ab") as f:
                f.write(b"\x00" * padding_size)

            # 更新文件大小和计算补零后的余数
            new_file_size = os.path.getsize(file_path)
            new_remainder = new_file_size % 4096

            # 打印补零信息
            print(f"文件：{file}，字节数：{file_size}，补零后文件大小：{new_file_size}，原始余数：{remainder}，补零后余数：{new_remainder}")

            # 替换原始的SZ_FILE所在的四个字节
            sz_file_index = VERSION_LEN + CHIP_LEN + DEVICETYPE_LEN + IDENT_LEN + EXT_FLAG_LEN + RES_LEN + OUTVERSION_LEN + OUTDATE_LEN + CRC_LEN
            replace_bytes(file_path, struct.pack("<I", new_file_size), sz_file_index, "sz_file")
            
            # 计算 sz_ih 之后的数据长度
            start_index = sz_file_index + SZ_FILE_LEN + SZ_IH_LEN + SZ_SH_LEN + NR_SH_LEN
            # 读取文件内容
            with open(file_path, "rb") as f:
                file_content = f.read()
            crc32_result = zlib.crc32(file_content[start_index:])

            # 替换原始的crc所在的四个字节
            crc_index = VERSION_LEN + CHIP_LEN + DEVICETYPE_LEN + IDENT_LEN + EXT_FLAG_LEN + RES_LEN + OUTVERSION_LEN + OUTDATE_LEN
            replace_bytes(file_path, struct.pack("<I", crc32_result), crc_index, "crc")

        else:
            # 打印不需要补零的信息
            print(f"文件：{file}，字节数：{file_size}，余数：{remainder}")
    else:
        # 如果文件不存在，打印错误信息
        print(f"文件：{file} 不存在！")
