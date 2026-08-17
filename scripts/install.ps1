[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$KenshiDirectory
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repoRoot 'dist\KenshiJobManagement'
$modsDirectory = Join-Path $KenshiDirectory 'mods'
$destination = Join-Path $modsDirectory 'KenshiJobManagement'

if (-not (Test-Path $source -PathType Container)) {
    throw "Build output was not found: $source. Run scripts\build.ps1 first."
}

$requiredFiles = @(
    'KenshiJobManagement.dll',
    'KenshiJobManagement.mod',
    'RE_Kenshi.json'
)
foreach ($file in $requiredFiles) {
    $path = Join-Path $source $file
    if (-not (Test-Path $path -PathType Leaf)) {
        throw "Build output is incomplete; missing: $path"
    }
}

New-Item -ItemType Directory -Path $modsDirectory -Force | Out-Null
if (Test-Path $destination) {
    Remove-Item $destination -Recurse -Force
}
New-Item -ItemType Directory -Path $destination -Force | Out-Null
Copy-Item -Path (Join-Path $source '*') -Destination $destination -Recurse -Force
$obsoleteInstalledReadme = Join-Path $destination 'README.txt'
if (Test-Path $obsoleteInstalledReadme -PathType Leaf) {
    Remove-Item $obsoleteInstalledReadme -Force
}
Write-Host "Installed to: $destination"
