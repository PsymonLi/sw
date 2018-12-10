import sys
import os

coverage_path = os.environ.get("COVERAGE_CONFIG_PATH")
if not coverage_path:
    print ("Coverage config path not set, please set COVERAGE_CONFIG_PATH")
    sys.exit(1)
coverage_path = os.path.abspath(coverage_path) + "/"

coverage_output_path = os.environ.get("COVERAGE_OUTPUT")
if not coverage_output_path:
    print ("Coverage output not set, please set COVERAGE_OUTPUT")
    sys.exit(1)
coverage_output_path = os.path.abspath(coverage_output_path) + "/"
asm_out_final = coverage_output_path + "asm_out_final"
p4_data_output_path = coverage_output_path + "p4_data"

nic_dir = os.environ.get("NIC_DIR")
if not nic_dir:
    print ("Nic not set, please set NIC_DIR")
    sys.exit(1)
nic_dir = os.path.abspath(nic_dir)
asm_src_dir = nic_dir + "/" + "asm"

capcov_cmd = os.environ.get("CAPCOV")
if not capcov_cmd:
    print ("Capcov command path not set, please set CAPCOV")
    sys.exit(1)

gcovr_cmd = os.environ.get("GCOVR")
if not gcovr_cmd:
    print ("Gcvor command path not set, please set GCOVR")
    sys.exit(1)

capri_loader_conf = os.environ.get("CAPRI_LOADER_CONF")
if not capri_loader_conf:
    print ("Capri loader conf path not set, please set CAPRI_LOADER_CONF")
    sys.exit(1)

gen_dir = os.environ.get("GEN_DIR")
if not gen_dir:
    print ("GEN Dir path not set, please set GEN_DIR")
    sys.exit(1)

bullseye_model_cov_file = os.environ.get("BULLSEYE_MODEL_COVERAGE_FILE", None)
bullseye_model_html_output_dir = os.environ.get("BULLSEYE_HTML_OUTPUT_DIR", None)

sim_bullseye_hal_cov_file = os.environ.get("SIM_BULLSEYE_HAL_COVERAGE_FILE", None)
bullseye_hal_filter_cov_file = os.environ.get("BULLSEYE_HAL_FILTER_COVERAGE_FILE", None)
sim_bullseye_hal_html_output_dir = os.environ.get("SIM_BULLSEYE_HAL_HTML_OUTPUT_DIR", None)

hw_bullseye_hal_cov_file = os.environ.get("HW_BULLSEYE_HAL_COVERAGE_FILE", None)
hw_bullseye_hal_html_output_dir = os.environ.get("HW_BULLSEYE_HAL_HTML_OUTPUT_DIR", None)
all_bullseye_hal_cov_file = os.environ.get("ALL_BULLSEYE_HAL_COVERAGE_FILE", None)
all_bullseye_hal_html_output_dir = os.environ.get("ALL_BULLSEYE_HAL_HTML_OUTPUT_DIR", None)
hw_run_bullseye_hal_dir = os.environ.get("HW_RUN_BULLSEYE_HAL_DIR", None)

bullseye_covhtml_cmd = os.environ.get("BULLSEYE_COVHTML")
if not bullseye_covhtml_cmd:
    print ("Bulls eye covhtml command path not set, please set BULLSEYE_COVHTML")
    sys.exit(1)

bullseye_covselect_cmd = os.environ.get("BULLSEYE_COVSELECT")
if not bullseye_covselect_cmd:
    print ("Bulls eye covselect command path not set, please set BULLSEYE_COVSELECT")
    sys.exit(1)

bullseye_covmerge_cmd = os.environ.get("BULLSEYE_COVMERGE")
if not bullseye_covmerge_cmd:
    print ("Bulls eye covmerge command path not set, please set BULLSEYE_COVMERGE")
    sys.exit(1)
