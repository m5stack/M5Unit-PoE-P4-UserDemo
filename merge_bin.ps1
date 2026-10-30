<#
.SYNOPSIS
    把 build_<变体> 目录里的 bootloader / 分区表 / 应用合并成单个可烧录 bin。

.DESCRIPTION
    偏移、flash 模式/频率/容量全部从 build_<变体>\flasher_args.json 自动读取，
    不写死，切换变体或改分区表后不用修改本脚本。

.PARAMETER Product
    p4（ESP32-P4 rev < 3.0）或 p4x（ESP32-P4 rev >= 3.0）。

.PARAMETER Output
    自定义输出路径；不填则使用下面 $OutputNames 里的文件名，输出到 firmware\ 目录。

.PARAMETER NoPad
    不把输出补齐到整片 flash 大小（默认会补齐，便于产线整片烧录）。

.EXAMPLE
    .\merge_bin.ps1 p4x

.EXAMPLE
    .\merge_bin.ps1 p4 -Output .\firmware\my_p4_test.bin
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('p4', 'p4x')]
    [string]$Product,

    [string]$Output,

    [switch]$NoPad
)

$ErrorActionPreference = 'Stop'

$ProjectRoot = $PSScriptRoot
$BuildDir    = Join-Path $ProjectRoot "build_$Product"

# ---------------------------------------------------------------------------
# 输出文件名：需要改名就改这里
# ---------------------------------------------------------------------------
$OutputNames = @{
    'p4'  = 'Unit-PoE-P4-UserDemo_p4_merged.bin'
    'p4x' = 'Unit-PoE-P4X-UserDemo_p4x_merged.bin'
}

if (-not $Output) {
    $Output = Join-Path (Join-Path $ProjectRoot 'firmware') $OutputNames[$Product]
}

$FlasherArgs = Join-Path $BuildDir 'flasher_args.json'
if (-not (Test-Path $FlasherArgs)) {
    throw "未找到 $FlasherArgs`n请先编译： idf.py -B build_$Product -DPRODUCT=$Product build"
}

$meta = Get-Content $FlasherArgs -Raw | ConvertFrom-Json
$chip = $meta.extra_esptool_args.chip
$mode = $meta.flash_settings.flash_mode
$freq = $meta.flash_settings.flash_freq
$size = $meta.flash_settings.flash_size

# 按偏移数值升序排列（0x2000 -> 0x8000 -> 0x10000 ...）
$offsets = $meta.flash_files.PSObject.Properties.Name |
    Sort-Object { [Convert]::ToInt64($_, 16) }

$arguments = @(
    '--chip', $chip,
    'merge-bin',
    '-o', $Output,
    '--flash-mode', $mode,
    '--flash-freq', $freq,
    '--flash-size', $size
)
if (-not $NoPad) {
    $arguments += @('--pad-to-size', $size)
}

foreach ($off in $offsets) {
    $arguments += $off
    $arguments += (Join-Path $BuildDir $meta.flash_files.$off)
}

$outDir = Split-Path -Parent $Output
if ($outDir -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

Write-Host "==> 变体   : $Product"
Write-Host "==> 芯片   : $chip  ($mode / $freq / $size)"
Write-Host "==> 输入   : $BuildDir"
Write-Host "==> 输出   : $Output"

& python -m esptool @arguments
if ($LASTEXITCODE -ne 0) {
    throw "esptool 执行失败（exit $LASTEXITCODE）。请确认已激活 ESP-IDF 环境（venv 中存在 esptool）。"
}

$merged = Get-Item $Output
Write-Host ("==> 完成   : {0} ({1:N2} MB)" -f $merged.FullName, ($merged.Length / 1MB))
