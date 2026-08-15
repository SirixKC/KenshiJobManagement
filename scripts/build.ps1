[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$solution = Join-Path $repoRoot 'KenshiJobManagement.sln'
$packageDirectory = Join-Path $repoRoot 'dist\KenshiJobManagement'
$packageArchive = Join-Path $repoRoot 'dist\KenshiJobManagement-0.1.0-alpha.zip'

function Require-Directory([string]$Path, [string]$Description) {
    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path -PathType Container)) {
        throw "$Description was not found: $Path"
    }
}

function Require-File([string]$Path, [string]$Description) {
    if (-not (Test-Path $Path -PathType Leaf)) {
        throw "$Description was not found: $Path"
    }
}

if ([string]::IsNullOrWhiteSpace($env:KENSHILIB_DIR)) {
    throw 'KENSHILIB_DIR is not set. Point it at the KenshiLib folder containing Include and Libraries.'
}

if ([string]::IsNullOrWhiteSpace($env:BOOST_INCLUDE_PATH)) {
    throw 'BOOST_INCLUDE_PATH is not set. Point it at the Boost root containing the boost headers.'
}

Require-Directory (Join-Path $env:KENSHILIB_DIR 'Include') 'KenshiLib Include directory'
Require-Directory (Join-Path $env:KENSHILIB_DIR 'Libraries') 'KenshiLib Libraries directory'
Require-Directory (Join-Path $env:BOOST_INCLUDE_PATH 'boost') 'Boost headers directory'
Require-File (Join-Path $env:KENSHILIB_DIR 'Libraries\KenshiLib.lib') 'KenshiLib.lib'
Require-File (Join-Path $env:KENSHILIB_DIR 'Libraries\MyGUIEngine_x64.lib') 'MyGUIEngine_x64.lib'
Require-File (Join-Path $env:KENSHILIB_DIR 'Libraries\OgreMain_x64.lib') 'OgreMain_x64.lib'

$msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue
if ($null -eq $msbuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $candidate = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if (-not [string]::IsNullOrWhiteSpace($candidate)) {
            $msbuild = Get-Item $candidate
        }
    }
}

if ($null -eq $msbuild) {
    throw 'MSBuild was not found. Run this script from a Visual Studio Developer PowerShell.'
}

& $msbuild.FullName $solution /m /t:Build /p:Configuration=Release /p:Platform=x64
if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed with exit code $LASTEXITCODE. Confirm the Visual C++ v100 x64 toolset is installed."
}

$requiredPackageFiles = @(
    'KenshiJobManagement.dll',
    'KenshiJobManagement.mod',
    'RE_Kenshi.json',
    'README.txt'
)

foreach ($file in $requiredPackageFiles) {
    Require-File (Join-Path $packageDirectory $file) 'Build package file'
}
Require-Directory (Join-Path $packageDirectory 'gui') 'Station icon package directory'
Get-ChildItem (Join-Path $repoRoot 'mod\gui\*.png') | ForEach-Object {
    Require-File (Join-Path $packageDirectory ('gui\' + $_.Name)) 'Station icon package file'
}

if (Test-Path $packageArchive) {
    Remove-Item $packageArchive -Force
}
Compress-Archive -Path $packageDirectory -DestinationPath $packageArchive -CompressionLevel Optimal

Write-Host "Build complete: $packageDirectory"
Write-Host "Test package:  $packageArchive"
