# =============================================================================
# 产品变体：Unit PoE-P4（ESP32-P4 rev < 3.0 / eco4）
#
# 构建：idf.py -B build_p4 -DPRODUCT=p4 build
#
# 与 P4X 的差异只有下面几类（芯片 revision 决定）：
#   - revision 开关
#   - CPU 主频上限
#   - PSRAM 速度上限
#   - 产品标识 CONFIG_EXAMPLE_POE_P4X
# =============================================================================

CONFIG_IDF_TARGET="esp32p4"

# 40 MHz 晶振（两个产品一致）
CONFIG_XTAL_FREQ_40=y

# 芯片 revision 低于 v3.0，最低支持 v0.0
CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y
CONFIG_ESP32P4_REV_MIN_0=y

# rev < 3.0 的时钟上限：CPU 360 MHz
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_360=y

# 外部 PSRAM：启用 + rev < 3.0 的速度上限 200 MHz
CONFIG_SPIRAM=y
CONFIG_SPIRAM_SPEED_200M=y

# 产品标识：P4（不是 P4X）
# CONFIG_EXAMPLE_POE_P4X is not set
