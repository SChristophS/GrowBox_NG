import os
import subprocess

def read_config():
    config = {}
    with open('config.txt', 'r') as file:
        lines = file.readlines()
        for line in lines:
            key, value = line.strip().split('=')
            config[key] = value
    return config

def run_command(command, log_file):
    with open(log_file, 'a') as log:
        process = subprocess.Popen(command, shell=True, stdout=log, stderr=log)
        process.communicate()

def main():
    config = read_config()
    source_path = config.get('source_path', '.')
    header_path = config.get('header_path', '.')
    output_log = config.get('output_log', 'output_log.log')

    # Clear the existing log file
    with open(output_log, 'w') as log:
        log.write("Running analysis tools...\n")

    # Run clang-tidy
    run_command(f'clang-tidy {source_path}/* -- -I{header_path}', output_log)

    # Run cppcheck
    run_command(f'cppcheck --enable=all --inconclusive -I {header_path} {source_path}', output_log)

    # Run Include-What-You-Use (IWYU)
    run_command(f'iwyu_tool.py -p . {source_path}/*.c', output_log)

    print(f"Analysis complete. Results are in {output_log}")

if __name__ == "__main__":
    main()
