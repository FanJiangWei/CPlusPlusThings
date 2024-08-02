# -*- coding: utf-8 -*-
import csv
import os
import sys
import datetime
import change_config_bin
def remove_empty_lines(file_name):
    with open(file_name, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    with open(file_name, 'w', encoding='utf-8') as file:
        for line in lines:
            if ",,,,,,,,,,,,,\n" not in line:
                file.write(line)

def find_and_build_data_dict(file_name, province_code):
    data_dict = {}

    matched_column_index = None
    with open(file_name, 'r', encoding='utf-8') as file:
        reader = csv.reader(file)
        for index, row in enumerate(reader):
            if row and province_code in row:
                matched_column_index = row.index(province_code)
                break

    if matched_column_index is not None:
        with open(file_name, 'r', encoding='utf-8') as file:
            reader = csv.reader(file)
            for row in reader:
                if row:
                    data_dict[row[0]] = row[matched_column_index]

    return data_dict

def delet_temp_file(file_name):
    try:
        os.remove(file_name)
        # print(f"临时文件 '{file_name}' 已成功删除")
    except OSError as e:
        print(f"删除文件时出错: {e}")

def print_data_dict(data_dict):
    for key, value in data_dict.items():
        print(f"{key}: {value}")

def find_value_by_key(data_dict, key_to_find):
    if key_to_find in data_dict:
        return data_dict[key_to_find]
    else:
        return None

def print_hex_bytes(byte_array_name, byte_array):
    hex_string = ' '.join(format(byte, '02X') for byte in byte_array)
    print(f"{byte_array_name} ({len(byte_array)} bytes): {change_config_bin.red_text}{hex_string}{change_config_bin.reset_color}")

def build_function_switch_byte(data_dict):
    # 定义开关名称列表，按顺序对应各个开关
    switch_names = [
        "function_switch_usemode", "DebugeMode", "NoiseDetectSWC", "WhitelistSWC", "UpgradeMode",
        "AODVSWC", "EventReportSWC", "ModuleIDGetSWC", "PhaseSWC", "IndentifySWC", "DataAddrFilterSWC",
        "NetSenseSWC", "SignalQualitySWC", "SetBandSWC", "SwPhase", "oop20EvetSWC", "oop20BaudSWC",
        "JZQBaudSWC", "MinCollectSWC", "IDInfoGetModeSWC", "TransModeSWC", "AddrShiftSWC"
    ]

    # 初始化字节列表
    bytes_list = [0] * 11

    for i, switch_name in enumerate(switch_names):
        switch_value = int(data_dict.get(switch_name, '0'), 16) & 0xFF
        # 将开关值位放入相应的字节位置，这里将开关值左移 i % 8 位
        bytes_list[i // 8] |= (switch_value << (i % 8))

    # 获取 function_switch_used 的值，并将十六进制字符串转换为一个字节
    used_value = int(data_dict.get("function_switch_used", '0'), 16) & 0xFF
    bytes_list[10] = used_value

    # 计算校验和
    checksum = sum(bytes_list[:11]) & 0xFF

    # 将校验和添加到字节列表末尾
    bytes_list.append(checksum)

    # 将字节列表转换为字节对象
    function_switch_bytes = bytes(bytes_list)

    #打印并返回结果
    print_hex_bytes("function_switch", function_switch_bytes)
    return function_switch_bytes

def build_parameter_cfg_byte(data_dict):
    # 定义字段名称列表，按顺序对应各个字段
    field_names = [
        "param_cfg_usemode", "ConcurrencySize", "ReTranmitMaxNum", "ReTranmitMaxTime",
        "BroadcastConRMNum", "AllBroadcastType", "JZQ_baud_cfg"
    ]

    # 初始化字节列表
    bytes_list = [0] * 19

    for i, field_name in enumerate(field_names):
        # 获取字段的值，并将十六进制字符串转换为一个字节
        field_value = int(data_dict.get(field_name, '0'), 16) & 0xFF
        # 将字段值放入相应的字节位置
        bytes_list[i] = field_value

    # 获取 param_cfg_used 的值，并将十六进制字符串转换为一个字节
    used_value = int(data_dict.get("param_cfg_used", '0'), 16) & 0xFF
    bytes_list[18] = used_value

    # 计算校验和
    checksum = sum(bytes_list[:19]) & 0xFF

    # 将校验和添加到字节列表末尾
    bytes_list.append(checksum)

    # 将字节列表转换为字节对象
    parameter_cfg_bytes = bytes(bytes_list)

    #打印并返回结果
    print_hex_bytes("parameter_cfg", parameter_cfg_bytes)
    return parameter_cfg_bytes

def get_current_bcd_date():
    # 获取系统当前时间的年、月、日
    current_date = datetime.datetime.now()
    current_year = current_date.year % 100  # 只取后两位
    current_month = current_date.month
    current_day = current_date.day

    # 将年、月、日转换为BCD码数组
    current_date_bcd = [
        int(str(current_day).zfill(2), 16),
        int(str(current_month).zfill(2), 16),
        int(str(current_year).zfill(2), 16)
    ]
    
    #打印并返回结果
    print_hex_bytes("current_date_bcd", current_date_bcd)
    return current_date_bcd

def create_fcs_tab():
    fcstab = []

    polynomial_value = 0x8408

    for b in range(256):
        v = b
        for i in range(8):
            v = (v >> 1) ^ polynomial_value if v & 1 else v >> 1
        fcstab.append(v & PPPINITFCS16)

    return fcstab

# 定义常量
PPPINITFCS16 = 0xFFFF

def tryfcs16(cp, length, fcstab):
    cp_copy = cp.copy()  # 复制原始的 cp 列表
    trialfcs = pppfcs16(PPPINITFCS16, cp_copy, length, fcstab)
    trialfcs ^= PPPINITFCS16
    cp.append(trialfcs & 0xFF)
    cp.append((trialfcs >> 8) & 0xFF)

    return trialfcs

def pppfcs16(fcs, cp, length, fcstab):
    cp_copy = cp.copy()  # 复制原始的 cp 列表
    while length > 0:
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ cp_copy[0]) & 0xFF]
        cp_copy.pop(0)
        length -= 1
    return fcs

def cal_crc16_valve(binary_data):

    fcstab = create_fcs_tab()

    # 计算 CRC
    crc_value = tryfcs16(list(binary_data), len(binary_data), fcstab)

    # 将 CRC 值拆分为两个字节（小端）
    crc_bytes = [(crc_value >> i) & 0xFF for i in range(0, 16, 8)]

    # 打印并返回结果
    print_hex_bytes("crc_value", crc_bytes)
    return crc_bytes  

def get_version_name(dev_type):
    version_mapping = {
        'cco_3951': ('cco_3951_version', 'cco_3951_out_version'),
        'cco_3951A': ('cco_3951A_version', 'cco_3951A_out_version'),
        'cco_venus': ('cco_venus_version', 'cco_venus_out_version'),
        'cco_233l': ('cco_233l_version', 'cco_233l_out_version'),
        'cco_riscv': ('cco_riscv_version', 'cco_riscv_out_version'),
        'sta_3750': ('sta_3750_version', 'sta_3750_out_version'),
        'sta_3750A': ('sta_3750A_version', 'sta_3750A_out_version'),
        'sta_riscv': ('sta_riscv_version', 'sta_riscv_out_version'),
        'sta_233l': ('sta_233l_version', 'sta_233l_out_version'),
        'sta_venus': ('sta_venus_version', 'sta_venus_out_version'),
        'cjq2_3750': ('cjq2_3750_version', 'cjq2_3750_out_version'),
        'cjq2_3750A': ('cjq2_3750A_version', 'cjq2_3750A_out_version'),
        'cjq2_riscv': ('cjq2_riscv_version', 'cjq2_riscv_out_version'),
        'cjq2_233l': ('cjq2_233l_version', 'cjq2_233l_out_version'),
        'cjq2_venus': ('cjq2_venus_version', 'cjq2_venus_out_version')
    }
    
    version_names = version_mapping.get(dev_type, (None, None))
    print(f"Dev Type: {dev_type}")
    print(f"Internal Version Name: {version_names[0]}")
    print(f"External Version Name: {version_names[1]}")
    
    return version_names

def write_to_bin_file(dev_type, config_array):
    prefix_map = {
        'cco_233l': 'ZCDBU',
        'cco_3750': 'ZCPJU',
        'cco_3750A': 'ZCPJU',
        'cco_venus': 'ZCDBU',
        'cco_riscv': 'ZCDBU',
        'cjq2_riscv': 'ZCDBC',
        'cjq2_233l': 'ZCDBC',
        'cjq2_venus': 'ZCDBC',
        'cjq2_3750': 'ZCPJC',
        'cjq2_3750A': 'ZCPJC',
        'sta_riscv': 'ZCDBM',
        'sta_233l': 'ZCDBY',
        'sta_venus': 'ZCDBM',
        'sta_3750': 'ZCPJM',
        'sta_3750A': 'ZCPJM'
    }
    
    if dev_type in prefix_map:
        prefix = prefix_map[dev_type]
        for filename in os.listdir('.'):
            if filename.startswith(prefix):
                bin_filename = filename
                print(f"Appending to {bin_filename}")  # 打印写入的 BIN 文件名
                
                with open(bin_filename, "ab+") as bin_file:
                    bin_file.seek(0, 2)  # 移动到文件末尾
                    bin_file.write(bytes(config_array))
                    print_hex_bytes("config_array", config_array)
                break  # 找到第一个匹配的文件后就停止查找
        else:
            print(f"No matching BIN file found for prefix {prefix}")
    else:
        print("未知的 dev_type 类型")

def build_config_bin(data_dict, function_switch_bytes, parameter_cfg_bytes, dev_type):
    # 获取内部版本名和外部版本名
    internal_version_name, out_version_name = get_version_name(dev_type)

    # 解析水印值并将其拆分为四个字节（小端字节序）
    water_mark_value = int(data_dict.get("water_mark", '0xaabbccdd'), 16)
    water_mark_bytes = [(water_mark_value >> i) & 0xFF for i in range(0, 32, 8)]

    # 解析版本号并将其拆分成两个字节（大端字节序）
    def parse_version(version_name):
        version_value_str = data_dict.get(version_name, '0xFFFF')
        version_value = int(version_value_str, 16) if version_value_str else 0xFFFF
        return [(version_value >> i) & 0xFF for i in range(0, 16, 8)]

    internal_version_bytes = parse_version(internal_version_name)
    print_hex_bytes("internal_version", internal_version_bytes)
    out_version_bytes = parse_version(out_version_name)
    print_hex_bytes("out_version", out_version_bytes)

    # 解析省份代码，并将其拆分为单个字节（大端字节序）
    province_code_value_str = data_dict.get('province_code', '0x00')
    province_code_value = int(province_code_value_str, 16) if province_code_value_str else 0x00
    province_code_byte = [province_code_value & 0xFF]

    # 获取系统当前时间的年、月、日
    date_hex_bytes = get_current_bcd_date()

    # 构建配置数组
    config_array = [
        *water_mark_bytes,
        *province_code_byte,
        *internal_version_bytes,
        *date_hex_bytes,
        *out_version_bytes,
        *date_hex_bytes,
        *function_switch_bytes,
        *parameter_cfg_bytes
    ]

    # 计算 config_array 数组的字节数
    config_byte_count = len(config_array) + 2

    # 将字节数转换为小端的两个字节
    config_byte_count_bytes = [(config_byte_count >> i) & 0xFF for i in range(0, 16, 8)]

    # 将字节数添加到 config_array 数组的起始位置
    config_array = config_byte_count_bytes + config_array
    
    # 计算 CRC16
    crc_bytes = cal_crc16_valve(config_array)

    # 将 CRC 值添加到数组起始位置
    config_array = crc_bytes + config_array

    # 确保数组长度为 512，不足时进行填充
    if len(config_array) < 512:
        padding = [0xFF] * (512 - len(config_array))
        config_array += padding

     # 写入到 BIN 文件
    write_to_bin_file(dev_type, config_array)


def get_dev_type(image_c):
    dev_types = {
        '2': ('cco_233l', 'sta_233l', 'cjq2_233l'),
        'u': ('cco_3750', 'sta_3750', 'cjq2_3750'),
        'r': ('cco_3750A', 'sta_3750A', 'cjq2_3750A'),
        'd': ('cco_venus', 'sta_venus', 'cjq2_venus'),
        'v': ('cco_riscv', 'sta_riscv', 'cjq2_riscv')
    }
    return dev_types.get(image_c)

def main():
    if len(sys.argv) != 3:
        print("用法：python3 make_config_bin.py <province_code> <image_c>")
        sys.exit(1)

    province_code = sys.argv[1]
    image_c = sys.argv[2]

    print(f"Province Code: {province_code}")
    print(f"Image C: {image_c}")

    file_name = 'config_data'

    # remove_empty_lines(file_name)
    data_dict = find_and_build_data_dict(file_name, province_code)
    # print_data_dict(data_dict)
    delet_temp_file(file_name)

    function_switch_bytes = build_function_switch_byte(data_dict)
    parameter_cfg_bytes = build_parameter_cfg_byte(data_dict)

    dev_types = get_dev_type(image_c)
    if dev_types:
        for dev_type in dev_types:
            build_config_bin(data_dict, function_switch_bytes, parameter_cfg_bytes, dev_type)
    else:
        print("未知的 image_c 类型")

if __name__ == "__main__":
    main()