# -*- coding: utf-8 -*-
import subprocess
import multiprocessing
import sys
import time

def run_make_command(command):
    """
    执行给定的 make 命令，并返回执行结果（包括标准输出和标准错误）。
    """
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    return result

def execute_commands(image_c, province):
    # 使用字典存储不同 image_c 值对应的命令列表
    command_dict = {
        "2": [
            ["make", "-j", "venus2m_233l_clt2", f"PROVINCE={province}"],
            ["make", "-j", "venus2m_233l_sta", f"PROVINCE={province}"],
            ["make", "-j", "venus8m_233l_cco", f"PROVINCE={province}"]
        ],
        "v": [
            ["make", "-j", "venus2m_riscv_clt2", f"PROVINCE={province}"],
            ["make", "-j", "venus2m_riscv_sta", f"PROVINCE={province}"]
        ],
        "d": [
            ["make", "-j", "venus2m_sta", f"PROVINCE={province}"],
            ["make", "-j", "venus2m_clt2", f"PROVINCE={province}"],
            ["make", "-j", "venus8m_cco", f"PROVINCE={province}"]
        ],
        "r": [
            ["make", "-j", "roland1_1m_sta", f"PROVINCE={province}"],
            ["make", "-j", "roland1_1m_clt2", f"PROVINCE={province}"],
            ["make", "-j", "roland9_1m_cco", f"PROVINCE={province}"]
        ],
        "u": [
            ["make", "-j", "unicorn2m_sta", f"PROVINCE={province}"],
            ["make", "-j", "unicorn2m_clt2", f"PROVINCE={province}"],
            ["make", "-j", "unicorn8m_cco", f"PROVINCE={province}"]
        ]
    }

    start_time = time.time()

    commands = command_dict.get(image_c)
    if commands is None:
        print(f"{image_c} 是什么东西？")
        sys.exit(1)

    # 使用 multiprocessing 并行执行多个命令
    with multiprocessing.Pool(processes=len(commands)) as pool:
        results = pool.map(run_make_command, commands)

    end_time = time.time()
    total_time = end_time - start_time

    for i, result in enumerate(results):
    #     print(f"Command {i + 1}:")
    #     print("---- stdout ----")
    #     print(result.stdout)
    #     print("---- stderr ----")
        print(result.stderr)
    #     print("=" * 40)

    print(f"编译总耗时：{total_time:.2f} 秒")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("使用方法：python3 script.py image_c province")
        sys.exit(1)
    
    image_c = sys.argv[1]
    province = sys.argv[2]
    print("多进程编译中，请等待...")
    execute_commands(image_c, province)
