# -*- coding: utf-8 -*-

import subprocess
import os
import sys
import datetime
import change_config_bin
import make_config_bin

# 定义数据类型与格式化字符串的映射
DATA_TYPE_FORMAT = {
    "BIN": "0x%02X",    # 16进制整数
    "BCD": "0x%02d",    # 10进制整数
    "STRING": "%c"      # 字符
}

def get_macro():

    image_c = sys.argv[2]

    macro = {
        '2': ('CHIP_D', 'STD_DUAL', 'VENUS2M'),
        'u': ('CHIP_U', 'UNICORN2M', 'UNICORN8M'),
        'r': ('CHIP_R', 'ROLAND1_1M', 'ROLAND9_1M'),
        'd': ('CHIP_D', 'STD_DUAL', 'VENUS2M'),
        'v': ('CHIP_V', 'STD_DUAL', 'VENUS2M')
    }
    return macro.get(image_c)

def generate_and_run_c_code():
    try:
        # 删除旧的可执行文件和 .c 文件
        if os.path.exists("get_macro_value"):
            os.remove("get_macro_value")
        if os.path.exists("../src/include/get_macro_value.c"):
            os.remove("../src/include/get_macro_value.c")

        # 定义 C 代码模板
        c_template = """
#include <stdio.h>
#include "version.h"

int main() {{
{output_statements}
    return 0;
}}
"""

        # 获取当前脚本所在目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        c_file_path = os.path.join(script_dir, "../src/include/get_macro_value.c")
        exe_file_path = os.path.join(script_dir, "get_macro_value")

        macro_values = {}  # 初始化一个空的字典

        # 构建要插入的输出语句
        output_statements = ""
        for macro_name, (data_type, _) in macro_info.items():
            format_string = DATA_TYPE_FORMAT.get(data_type, "%s")
            output_statements += f'    printf("{macro_name} : {format_string}\\n", {macro_name});\n'

        # 将模板中的占位符替换为输出语句
        c_code = c_template.format(output_statements=output_statements)

        # 将生成的 C 代码写入文件
        with open(c_file_path, "w") as c_file:
            c_file.write(c_code)

        # 调用函数并传入变量名
        STD = find_and_assign_std_variable("STD")

        # 编译生成的 C 代码
        macro_tuple = get_macro()
        if macro_tuple:
            compile_command = ["gcc", c_file_path, f"-D{STD}", f"-D{macro_tuple[0]}", f"-D{macro_tuple[1]}", f"-D{macro_tuple[2]}", "-o", exe_file_path]
            subprocess.run(compile_command, check=True)

        # 运行生成的可执行文件并捕获输出
        run_command = [exe_file_path]
        completed_process = subprocess.run(run_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        output = completed_process.stdout

        # 将输出整理为字典
        for line in output.split('\n'):
            parts = line.split(" : ")
            if len(parts) == 2:
                macro_values[parts[0].strip()] = parts[1].strip()

        return macro_values  # 返回整理后的字典

    except subprocess.CalledProcessError as e:
        print("C 程序运行失败:", e.stderr)
    except Exception as e:
        print("发生错误:", e)
    finally:
        # 清理临时生成的文件
        for temp_file in [exe_file_path, c_file_path]:
            if os.path.exists(temp_file):
                os.remove(temp_file)

def find_and_assign_std_variable(variable_name):
    # 初始化变量
    variable = None

    # 打开文件并逐行查找匹配的字符串
    file_path = "./Makefile"
    with open(file_path, "r") as file:
        for line in file:
            if "STD_2016" in line:
                variable = "STD_2016"
                break  # 找到后即可退出循环
            elif "STD_GD" in line:
                variable = "STD_GD"
                break  # 找到后即可退出循环

    print(f"{variable_name} : {change_config_bin.red_text}{variable}{change_config_bin.reset_color}")
    return variable

def build_bin_by_version(src_image_name, dest_version, image_header_array):
    if src_image_name is None or dest_version is None:
        print(f"{change_config_bin.red_text}错误：参数不能为空{change_config_bin.reset_color}")
        return

    src_image_filename = f"image_{src_image_name}.bin"
    dest_version_filename = f"{dest_version}.bin"

    try:
        with open(src_image_filename, "rb") as src_image_file:
            dest_image_content = src_image_file.read()

        with open(dest_version_filename, "wb") as dest_version_file:
            dest_version_file.write(dest_image_content)  # 先写入原始image
            if len(image_header_array) != 20:
                print(f"{change_config_bin.red_text}错误：image_header_array的长度必须为20字节{change_config_bin.reset_color}")
                return
            else:
                dest_version_file.seek(0)  # 将文件指针移至文件开头
                dest_version_file.write(image_header_array)  # 覆盖写入image_header

        print(f"构建正式程序：{change_config_bin.red_text}{dest_version_filename}{change_config_bin.reset_color}")
        make_config_bin.print_hex_bytes("Add image header : ", image_header_array)
    except FileNotFoundError as fnfe:
        print(f"{change_config_bin.red_text}警告：无法找到文件 '{src_image_filename}'{change_config_bin.reset_color}")


     
# 定义要输出的宏信息：key为宏名称，value为数据类型和数据的值
macro_info = {
    "ZC3951CCO_ver1": ("BIN", None),
    "ZC3951CCO_ver2": ("BIN", None),
    "ZC3951CCO_Innerver1": ("BIN", None),
    "ZC3951CCO_Innerver2": ("BIN", None),
    "ZC3951CCO_chip1": ("STRING", None),
    "ZC3951CCO_chip2": ("STRING", None),
    "ZC3750CJQ2_ver1": ("BIN", None),
    "ZC3750CJQ2_ver2": ("BIN", None),
    "ZC3750CJQ2_Innerver1": ("BIN", None),
    "ZC3750CJQ2_Innerver2": ("BIN", None),
    "ZC3750CJQ2_chip1": ("STRING", None),
    "ZC3750CJQ2_chip2": ("STRING", None),
    "ZC3750STA_ver1": ("BIN", None),
    "ZC3750STA_ver2": ("BIN", None),
    "ZC3750STA_Innerver1": ("BIN", None),
    "ZC3750STA_Innerver2": ("BIN", None),
    "ZC3750STA_chip1": ("STRING", None),
    "ZC3750STA_chip2": ("STRING", None),
    "PRODUCT_func": ("STRING", None),
    "CHIP_code": ("STRING", None),
    "POWER_OFF": ("STRING", None),
    "ZC3951CCO_type": ("STRING", None),
    "ZC3951CCO_prtcl": ("BIN", None),
    "ZC3951CCO_prdct": ("BIN", None),
    "ZC3951CCO_UpDate_type": ("STRING", None),
    "ZC3750CJQ2_type": ("STRING", None),
    "ZC3750CJQ2_prtcl": ("BIN", None),
    "ZC3750CJQ2_prdct": ("BIN", None),
    "ZC3750CJQ2_UpDate_type": ("STRING", None),
    "ZC3750STA_type": ("STRING", None),
    "ZC3750STA_prtcl": ("BIN", None),
    "ZC3750STA_prdct": ("BIN", None),
    "ZC3750STA_UpDate_type": ("STRING", None),
    "PROPERTY": ("STRING", None),
    "Vender1": ("STRING", None),
    "Vender2": ("STRING", None),
    "Date_Y": ("BCD", None),
    "Date_M": ("BCD", None),
    "Date_D": ("BCD", None),
    "InnerDate_Y": ("BCD", None),
    "InnerDate_M": ("BCD", None),
    "InnerDate_D": ("BCD", None)
    }

def calculate_dateymd():

    dateY = int(macro_info.get("Date_Y").replace("0x", ""))
    dateM = int(macro_info.get("Date_M").replace("0x", ""))
    dateD = int(macro_info.get("Date_D").replace("0x", ""))

    # 计算dateymd值，通过按位与、左移等位运算来合并年月日信息
    dateymd = (dateY & 0x7F) | ((dateM << 7) & 0x780) | ((dateD << 11) & 0xF800)
    
    # 提取dateymd的低8位作为dateymd1
    dateymd1 = dateymd & 0xFF
    
    # 将dateymd的第9到16位右移8位，并提取低8位作为dateymd2
    dateymd2 = (dateymd >> 8) & 0xFF

    # 返回计算结果
    return dateymd1, dateymd2

def build_image_header(Innerver1, Innerver2, UpDate_type, out_ver2, out_ver1):

    image_identity_dict = {
    '2': bytes([0x5A, 0x43, 0x48, 0x43, 0x33, 0x37, 0x38, 0x30, 0x31, 0x00]),
    'v': bytes([0x5A, 0x43, 0x48, 0x43, 0x33, 0x37, 0x38, 0x30, 0x30, 0x00]),
    'd': bytes([0x74, 0x72, 0x69, 0x64, 0x75, 0x63, 0x74, 0x6F, 0x72, 0x00]),
    'r': bytes([0x5A, 0x43, 0x48, 0x43, 0x33, 0x37, 0x35, 0x30, 0x41, 0x00]),
    'u': bytes([0x74, 0x72, 0x69, 0x64, 0x75, 0x63, 0x74, 0x6F, 0x72, 0x00]),
    }

    image_c = sys.argv[2]

    if image_c in image_identity_dict:
        image_identity = image_identity_dict[image_c]

    ext_info_flag = bytes([int(sys.argv[3])])

    dateymd1, dateymd2 = calculate_dateymd()

    # 将所有数组连接在一起并转换为字节
    image_header_array = (
        bytes([Innerver1])
        + bytes([Innerver2])
        + bytes([ord(macro_info.get("CHIP_code"))])
        + bytes([UpDate_type])
        + image_identity
        + ext_info_flag
        + bytes([0x00])
        + bytes([out_ver2])
        + bytes([out_ver1])
        + bytes([dateymd1])
        + bytes([dateymd2])
    )

    return image_header_array

def generate_version(product_type, product_protocol, product_innerver1, product_innerver2, prdct):
    
    # 获取当前时间
    current_time = datetime.datetime.now()

    province_code = sys.argv[1]

    version = (
        macro_info.get("Vender1")
        + macro_info.get("Vender2")
        + macro_info.get("PRODUCT_func")
        + macro_info.get("CHIP_code")
        + product_type
        + "-"
        + product_protocol.replace("0x", "")
        + macro_info.get("POWER_OFF")
        + prdct.replace("0x", "")
        + "-"
        + "R"
        + product_innerver1.replace("0x", "")
        + product_innerver2.replace("0x", "")
        + macro_info.get("PROPERTY")
        + "."
        + province_code.replace("0x", "")
        + "-"
        + macro_info.get("InnerDate_Y").replace("0x", "")
        + macro_info.get("InnerDate_M").replace("0x", "")
        + macro_info.get("InnerDate_D").replace("0x", "")
        + current_time.strftime("%H%M")  # 使用strftime格式化为两位数
    )
    return version

def build_all_bins():
    # 构建版本号
    cjq2_version = generate_version(
        macro_info.get("ZC3750CJQ2_type"), 
        macro_info.get("ZC3750CJQ2_prtcl"), 
        macro_info.get("ZC3750CJQ2_Innerver1"), 
        macro_info.get("ZC3750CJQ2_Innerver2"),
        macro_info.get("ZC3750CJQ2_prdct")
        )

    sta_version = generate_version(
        macro_info.get("ZC3750STA_type"), 
        macro_info.get("ZC3750STA_prtcl"), 
        macro_info.get("ZC3750STA_Innerver1"), 
        macro_info.get("ZC3750STA_Innerver2"),
        macro_info.get("ZC3750STA_prdct")
        )

    cco_version = generate_version(
        macro_info.get("ZC3951CCO_type"), 
        macro_info.get("ZC3951CCO_prtcl"), 
        macro_info.get("ZC3951CCO_Innerver1"), 
        macro_info.get("ZC3951CCO_Innerver2"),
        macro_info.get("ZC3951CCO_prdct")
        )

    image_dict = {
        '2': ('venus2m_233l_clt2', 'venus2m_233l_sta', 'venus8m_233l_cco'),
        'v': ('venus2m_riscv_clt2', 'venus2m_riscv_sta', None),
        'd': ('venus2m_clt2', 'venus2m_sta', 'venus8m_cco'),
        'r': ('roland1_1m_clt2', 'roland1_1m_sta', 'roland9_1m_cco'),
        'u': ('unicorn2m_clt2', 'unicorn2m_sta', 'unicorn8m_cco'),
    }

    def process_image(image_name, version_info):
        
        image_header_array = build_image_header(
                int(macro_info.get(version_info["innerver1"]), 16), 
                int(macro_info.get(version_info["innerver2"]), 16), 
                ord(macro_info.get(version_info["update_type"])), 
                int(macro_info.get(version_info["ver2"]), 16), 
                int(macro_info.get(version_info["ver1"]), 16)
            )
        build_bin_by_version(image_name, version_info["version"], image_header_array)


    image_c = sys.argv[2]

    if image_c in image_dict:
        cjq2_image, sta_image, cco_image = image_dict[image_c]

        if cco_image:
            process_image(cco_image, {
                "version": cco_version,
                "innerver1": "ZC3951CCO_Innerver1",
                "innerver2": "ZC3951CCO_Innerver2",
                "update_type": "ZC3951CCO_UpDate_type",
                "ver2": "ZC3951CCO_ver2",
                "ver1": "ZC3951CCO_ver1"
            })

        if cjq2_image:
            process_image(cjq2_image, {
                "version": cjq2_version,
                "innerver1": "ZC3750CJQ2_Innerver1",
                "innerver2": "ZC3750CJQ2_Innerver2",
                "update_type": "ZC3750CJQ2_UpDate_type",
                "ver2": "ZC3750CJQ2_ver2",
                "ver1": "ZC3750CJQ2_ver1"
            })

        if sta_image:
            process_image(sta_image, {
                "version": sta_version,
                "innerver1": "ZC3750STA_Innerver1",
                "innerver2": "ZC3750STA_Innerver2",
                "update_type": "ZC3750STA_UpDate_type",
                "ver2": "ZC3750STA_ver2",
                "ver1": "ZC3750STA_ver1"
            })
    else:
        print(f"错误：无效的 image_c: {image_c}")

if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("用法：python3 ./get_define_value.py <province_code> <image_c> <ext_info_flag>")
        sys.exit(1)

    # 获取C程序输出的字典
    macro_values = generate_and_run_c_code()  

    # 更新macro_info字典中的value
    for macro_name, (_, _) in macro_info.items():
        if macro_name in macro_values:
            macro_info[macro_name] = macro_values[macro_name]

    # 打印更新后的macro_info
    # print("\n更新后的macro_info:")
    for key, value in macro_info.items():
        print(f"{key}: {change_config_bin.red_text}{value}{change_config_bin.reset_color}")    

    build_all_bins()



    


