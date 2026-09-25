param([Parameter(Mandatory=$false)][string]$Port)
$ErrorActionPreference="Stop"
$Idf="C:\esp\v6.1\esp-idf"
if(!(Test-Path "$Idf\export.ps1")){ throw "ESP-IDF 6.1 introuvable: $Idf" }
. "$Idf\export.ps1"
Set-Location (Join-Path $PSScriptRoot "..\firmware\master")
idf.py set-target esp32s3
idf.py reconfigure
idf.py -j 1 build
if($Port){ idf.py -p $Port flash }
