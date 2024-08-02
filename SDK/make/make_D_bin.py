# -*- coding: utf-8 -*-

import os
import sys
import shutil

# 定义一个字典，存储不同 image_c 对应的 master 和 node BOOT 文件名
boot_files = {
    "u": {
        "master": "ZBDN190820V0210.bin",
        "node": "ZBHn190820V0210.bin",
        "targets": {
            "master": ["ZCPJU"],
            "node": ["ZCPJM", "ZCPJC"]
        }
    },
    "r": {
        "master": "ZBDN210603V0340.bin",
        "node": "ZBHn210902V0350.bin",
        "targets": {
            "master": ["ZCPJU"],
            "node": ["ZCPJM", "ZCPJC"]
        }
    },
    "d": {
        "master": "boot_venus8m.bin",
        "node": "boot_venus2m.bin",
        "targets": {
            "master": ["ZCDBU"],
            "node": ["ZCDBC", "ZCDBM"]
        }
    },
    "v": {
        "master": "boot_venus2m_riscv.bin",
        "node": None,
        "targets": {
            "master": [],
            "node": []
        }
    },
    "2": {
        "master": "ZBDO221031V0010.bin",
        "node": "ZBHo230721V0020.bin",
        "targets": {
            "master": ["ZCDBU"],
            "node": ["ZCDBC", "ZCDBY"]
        }
    },
    # 可继续添加其他 image_c 对应的 master 和 node BOOT 文件名
}

# 定义 BOOT 文件大小宏
BOOT_SIZE = 0x60000

def insert_padded_into_target(target_prefix, boot_file_path):
    # 获取以指定前缀开头的目标文件列表
    target_files = [f for f in os.listdir() if f.startswith(target_prefix)]
    
    # 打开 BOOT 文件，读取内容
    with open(boot_file_path, 'rb') as boot_file:
        boot_content = boot_file.read()
        
        # 针对每个目标文件进行处理
        for target_file in target_files:
            # 找到最后一个大写的A或L的索引
            last_a_index = target_file.rfind('A')
            last_l_index = target_file.rfind('L')
            last_upper_index = max(last_a_index, last_l_index)
            
            if last_upper_index != -1:
                # 构建新文件的路径
                new_file_path = target_file[:last_upper_index] + 'D' + target_file[last_upper_index + 1:]
                
                # 创建并打开新文件，填充起始位置
                with open(new_file_path, 'wb') as new_file:

                    # 在新文件起始位置追加写入 BOOT_SIZE 个 0xFF
                    new_file.write(b'\xFF' * BOOT_SIZE)
        
                    # 将 BOOT 文件内容覆盖写入新文件的起始位置
                    new_file.seek(0)  # 移动文件指针到起始位置
                    new_file.write(boot_content)

                    # 打开源文件并复制内容到新文件的 BOOT 之后
                    with open(target_file, 'rb') as source_file:
                        new_file.seek(BOOT_SIZE)  # 移动文件指针到BOOT_SIZE
                        shutil.copyfileobj(source_file, new_file)
    
                print("已创建 D 版程序:", new_file_path)  # 打印新创建的 "D" 版本文件名

def move_zc_bin_files():
    # 目标文件夹
    target_folder = "../STD_2016bin"
    
    # 如果目标文件夹不存在，则创建它
    if not os.path.exists(target_folder):
        os.makedirs(target_folder)
    
    # 获取当前路径下以大写字母ZC开头的所有.bin文件
    zc_bin_files = [f for f in os.listdir() if f.startswith("ZC") and f.lower().endswith(".bin")]

    # 移动文件到目标文件夹
    for zc_bin_file in zc_bin_files:
        source_path = os.path.join(os.getcwd(), zc_bin_file)
        target_path = os.path.join(target_folder, zc_bin_file)
        shutil.move(source_path, target_path)
        print(f"移动 {zc_bin_file} 到 {target_folder}")

def main():
    # 从命令行参数获取 image_c 值
    image_c = sys.argv[1].lower()

    if image_c in boot_files:
        boot_info = boot_files[image_c]
        master_boot_name = boot_info["master"]
        node_boot_name = boot_info["node"]
        target_info = boot_info["targets"]

        # 构建 master 和 node BOOT 文件的路径
        master_boot_path = os.path.join("..", "BOOT", master_boot_name)
        node_boot_path = None if node_boot_name is None else os.path.join("..", "BOOT", node_boot_name)

        print("Master BOOT 文件路径:", master_boot_path)
        if node_boot_name is not None:
            print("Node BOOT 文件路径:", node_boot_path)

        if os.path.exists(master_boot_path):
            for target_prefix in target_info["master"]:
                insert_padded_into_target(target_prefix, master_boot_path)

            if node_boot_path is not None and os.path.exists(node_boot_path):
                for target_prefix in target_info["node"]:
                    insert_padded_into_target(target_prefix, node_boot_path)
        else:
            print(f"{master_boot_name} 不存在，请将它添加到 ../BOOT 目录中。")
        
        # 移动文件到STD_2016bin文件夹
        move_zc_bin_files()

    else:
        print("无效的 image_c 值。")

# 调用主函数
if __name__ == "__main__":
    main()
