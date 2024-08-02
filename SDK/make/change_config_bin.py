# -*- coding: utf-8 -*-
import os
import re
from make_config_bin import print_hex_bytes
import subprocess
from make_D_bin import move_zc_bin_files
import shutil

def read_ext_info(file_path):
    try:
        with open(file_path, 'rb') as file:
            file.seek(-512, os.SEEK_END)
            data = file.read(512)
            return data
    except Exception as e:
        print(f"读取文件 {file_path} 的末尾512字节时出错：{e}")
        return None

def parse_data(data):
    if len(data) == 512:
        watermark = data[4:8][::-1]  # 将水印从小端序转换为大端序
        return watermark
    return None

def remove_ext_info(file_path):
    try:
        with open(file_path, 'rb+') as file:
            file.seek(-512, os.SEEK_END)
            file.truncate()
            print(f"删除文件 {file_path} 的末尾512字节成功")
    except Exception as e:
        print(f"删除文件 {file_path} 的末尾512字节时出错：{e}")

def set_image_c(input_value):
    chip_types = {
        "": ("2", "芯片类型:233L"),
        "0": ("v", "芯片类型:RISC-V"),
        "1": ("d", "芯片类型:Venus"),
        "2": ("r", "芯片类型:3750A"),
        "3": ("u", "芯片类型:3750")
    }

    if input_value in chip_types:
        image_c, chip_name = chip_types[input_value]
        print(chip_name)
    else:
        print(f"{input_value} 是什么东西？")
        image_c = None
    
    return image_c

# 使用 ANSI 转义序列添加红色文本
red_text = "\033[91m"  # 91 表示红色代码
reset_color = "\033[0m"  # 重置颜色

def get_province_code_and_chip_type():

    # 使用 input() 函数获取用户输入
    input_province_code = input(f"{red_text}请输入目标配置信息的省份代码，回车默认为 00 (国网):{reset_color}")

    # 如果用户没有输入，则默认为 0
    if not input_province_code:
        print("未选择选项，默认为 00 (国网)...")
        input_province_code = "00"
    else:
        # 将输入转换为十进制整数并添加前导零
        input_province_code = f"{int(input_province_code):02}"
    
    # 使用 input() 函数获取用户输入
    input_chip_type = input(f"{red_text}请输入芯片类型（输入 0 选择 RISC-V，输入 1 选择 Venus，输入 2 选择 R (1M)，输入 3 选择 U (2M)，回车默认为 233L）:{reset_color}")

    return input_province_code, input_chip_type

def modify_bin_filenames(det_province_code):
    current_path = os.getcwd()

    for filename in os.listdir(current_path):
        if filename.startswith("ZC") and filename.endswith(".bin"):
            file_path = os.path.join(current_path, filename)  # 获取文件的完整路径
            new_filename = re.sub(r'\.00-', f'.{det_province_code}-', filename)
            new_file_path = os.path.join(current_path, new_filename)  # 新文件名的完整路径
            os.rename(file_path, new_file_path)
            print(f"已修改文件名: {filename} -> {new_filename}")

def create_backup_folder():
    backup_folder = os.path.join(os.getcwd(), "原始文件备份")
    if not os.path.exists(backup_folder):
        os.makedirs(backup_folder)

    for filename in os.listdir(os.getcwd()):
        if filename.startswith("ZC") and filename.endswith(".bin"):
            file_path = os.path.join(os.getcwd(), filename)
            backup_path = os.path.join(backup_folder, filename)
            shutil.copy(file_path, backup_path)
            print(f"已备份文件: {filename} -> {backup_path}")

def change_config_info():
    det_province_code, chip_type = get_province_code_and_chip_type()
    print(f"目标省份代码: {det_province_code}")
    
    chip_type = set_image_c(chip_type)

    # 绕开加密软件，shell可以直接读取，但是PYTHON读取报错
    subprocess.run("cat province_config_table.csv > config_data", shell=True)

    subprocess.call(["python3", "./make_config_bin.py", f"0x{det_province_code}", chip_type])

    modify_bin_filenames(det_province_code)
    move_zc_bin_files()

def main():
    current_path = os.getcwd()

    print(f"请将要变更配置信息的BIN文件复制到{red_text}make{reset_color}文件夹下按回车继续")
    input("按回车键继续...")

    create_backup_folder()

    for filename in os.listdir(current_path):
        if filename.startswith("ZC") and filename.endswith(".bin"):
            file_path = os.path.join(current_path, filename)
            data = read_ext_info(file_path)
            print_hex_bytes("ext_info", data)
            
            if data:
                watermark = parse_data(data)
                if watermark == b'\xAA\xBB\xCC\xDD':
                    print(f"文件 {filename}, 校验通过, 开始添加扩展信息。")
                    remove_ext_info(file_path)
                else:
                    print(f"文件 {filename}, 水印校验失败")
                    print_hex_bytes("watermark", watermark)
                    exit()

    change_config_info()

# 调用主函数
if __name__ == "__main__":
    main()
